#include <Arduino.h>
#include "../include/piscadas.h"

#include "../include/estabilidade.h"
#include "../include/ble_hid.h"
#include "../include/modos.h"

#define NUM_LEITURAS 50
#define DELTA_PISCADA      10
#define FATOR_RELEASE      0.35
#define TIMEOUT_PISCADA_MS 900
#define VELOCIDADE_MIN_PISCADA  6.0

#define ALPHA_MIN       0.03
#define ALPHA_MAX       0.30
#define ERRO_MIN        5.0
#define ERRO_MAX        60.0

static unsigned int vetorLeituras[NUM_LEITURAS];
static int indexLeituras     = 0;
static bool bufferPreenchido = false;
static float g_mediaAtual    = 0;
static float g_valorBaseLine = -1;
static float g_mediaAnterior = 0;
static bool emPiscada = false;
static unsigned long tempoInicioPiscada = 0;
static unsigned long ultimoDebug = 0;

unsigned long ultimoSampleUs = 0;

// 1) Guarda a leitura no buffer circular e marca quando ele enche pela 1ª vez.
static void registrarLeitura(int leitura) {
  vetorLeituras[indexLeituras] = leitura;
  indexLeituras = (indexLeituras + 1) % NUM_LEITURAS;
  if (!bufferPreenchido && indexLeituras == 0) bufferPreenchido = true;
}

// 2) Média simples das leituras do buffer.
static float calcularMediaLeituras() {
  unsigned long soma = 0;
  for (int i = 0; i < NUM_LEITURAS; i++) soma += vetorLeituras[i];
  return (float)soma / NUM_LEITURAS;
}

// 3) Inicializa o baseline na primeira vez que o buffer está cheio.
//    Retorna true se inicializou neste ciclo (o chamador deve encerrar o ciclo).
static bool inicializarBaselineSePossivel() {
  if (g_valorBaseLine < 0 && bufferPreenchido) {
    g_valorBaseLine = g_mediaAtual;
    g_mediaAnterior = g_mediaAtual;
    Serial.print("Baseline inicializado: ");
    return true;
  }
  return false;
}

// 4) Baseline ainda não existe?
static bool baselineNaoInicializado() {
  return (g_valorBaseLine < 0);
}

// 5) Debug periódico (a cada 150 ms).
static void imprimirDebugPeriodico(int leitura, bool btConectado) {
  if (millis() - ultimoDebug > 150) {
    float vel = g_mediaAnterior - g_mediaAtual;
    Serial.print("Leit:");   Serial.print(leitura);
    Serial.print(" Media:"); Serial.print(g_mediaAtual, 1);
    Serial.print(" Base:");  Serial.print(g_valorBaseLine, 1);
    Serial.print(" Vel:");   Serial.print(vel, 1);
    Serial.print(" Estado:"); Serial.print(emPiscada ? "PISCADA" : "normal");
    Serial.print(" Cursor:"); Serial.print(cursorEstavel);
    Serial.print(" Acumulado cursor:"); Serial.print(acumuladoMovimento);
    Serial.print(" Modo:");  Serial.print(nomeModo[modoAtual]);
    Serial.print(" BT:");    Serial.println(btConectado ? "OK" : "---");
    ultimoDebug = millis();
  }
}

// 6) Atualiza o baseline com alpha adaptativo conforme o erro.
static void atualizarBaselineAdaptativo() {
  float erro = abs(g_mediaAtual - g_valorBaseLine);
  float t = (erro - ERRO_MIN) / (ERRO_MAX - ERRO_MIN);
  if (t < 0.0) t = 0.0;
  if (t > 1.0) t = 1.0;
  float alphaAdaptativo = ALPHA_MIN + t * (ALPHA_MAX - ALPHA_MIN);

  g_valorBaseLine = (g_valorBaseLine * (1.0 - alphaAdaptativo))
                  + (g_mediaAtual    *  alphaAdaptativo);
}

// 7) Dispara o PRESS e registra no serial.
static void iniciarPiscada(float velocidadeQueda, int leitura) {
  emPiscada = true;
  tempoInicioPiscada = millis();
  botoesAtuais = 0x01;
  enviarReport(botoesAtuais, 0, 0);
  Serial.println(">>> PRESS");
  Serial.print("velocidade no momento: ");
  Serial.println(velocidadeQueda);
  Serial.print("Leit no momento:");   Serial.println(leitura);
  Serial.print("Media no momento:"); Serial.println(g_mediaAtual);
  Serial.print("Base no momento:");  Serial.println(g_valorBaseLine);
}

// 8) Log do ramo "tudo ok menos a velocidade".
static void registrarVelocidadeInsuficiente(float velocidadeQueda) {
  Serial.println("Tudo ok menos a velocidade");
  Serial.print("velocidade no momento: ");
  Serial.println(velocidadeQueda);
}

// 9) Estado normal: atualiza baseline e verifica se deve disparar o PRESS.
static void processarEstadoNormal(int leitura) {
  atualizarBaselineAdaptativo();

  bool quedaSuficiente = (g_mediaAtual < (g_valorBaseLine - DELTA_PISCADA));

  // frequencia de medias iguais, entao a velocidade e a diferença das medias
  float velocidadeQueda = g_mediaAnterior - g_mediaAtual;
  bool quedaBrusca = (velocidadeQueda > VELOCIDADE_MIN_PISCADA);

  // Cursor estabilizado
  bool cursorOk = cursorEstavelConfirmado();

  if (quedaSuficiente && quedaBrusca && cursorOk) {
    if (quedaBrusca) {
      iniciarPiscada(velocidadeQueda, leitura);
    } else {
      registrarVelocidadeInsuficiente(velocidadeQueda);
    }
  }
}

// 10) Libera o clique e registra o motivo.
static void finalizarPiscada(bool timeout) {
  emPiscada = false;
  botoesAtuais = 0x00;
  enviarReport(botoesAtuais, 0, 0);

  if (timeout) {
    Serial.println("<<< TimeOut");
  } else {
    Serial.println("<<< RELEASE");
  }
}

// 11) Estado de piscada: verifica retorno ao normal ou timeout.
static void processarEstadoPiscada() {
  // se a media volta
  bool voltouAoNormal = (g_mediaAtual > (g_valorBaseLine - DELTA_PISCADA * FATOR_RELEASE));
  bool timeout = (millis() - tempoInicioPiscada > TIMEOUT_PISCADA_MS);

  if (voltouAoNormal || timeout) {
    finalizarPiscada(timeout);
  }
}

void processarLeituraIR(int leitura, bool btConectado) {
  registrarLeitura(leitura);
  g_mediaAtual = calcularMediaLeituras();

  if (inicializarBaselineSePossivel()) return;
  if (baselineNaoInicializado()) return;

  imprimirDebugPeriodico(leitura, btConectado);

  if (!emPiscada) {
    processarEstadoNormal(leitura);
  } else {
    processarEstadoPiscada();
  }

  g_mediaAnterior = g_mediaAtual;
}

