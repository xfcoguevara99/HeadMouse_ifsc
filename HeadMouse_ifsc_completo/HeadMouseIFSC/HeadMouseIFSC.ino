#include <Wire.h>
#include "include/config.h"
#include "include/modos.h"
#include "include/ble_hid.h"
#include "include/energia.h"
#include "include/estabilidade.h"
#include "include/movimento.h"
#include "include/piscadas.h"

void setup() {
  Serial.begin(115200);
  configurarEnergia();
  Wire.begin(pinoSDA, pinoSCL);
  pinMode(PIN_BOOT, INPUT_PULLUP);
  //memset(vetorLeituras, 0, sizeof(vetorLeituras));
  Serial.println("Iniciando MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050: OK");
  } else {
    Serial.println("MPU6050: Erro de conexao!");
  }
  setCalibrationMPU();
  Serial.println("Iniciando BLE...");
  setupBLE();
  tempoSemConexao = millis();
  ultimoMPU       = millis();
  float acel_x = mpu.getAccelerationX();
  atualizarOrientacao(acel_x);
  Serial.print("HeadMouse iniciado - CPU: 50MHz - Modo: ");
  Serial.println(nomeModo[modoAtual]);
}

void loop() {
  unsigned long agoraMilis = millis();
  // ── Botão BOOT: troca de modo ─────────────────────────
  if (digitalRead(PIN_BOOT) == LOW) {
    if (agoraMilis - ultimaTrocaModo > DEBOUNCE_MS) {
      ultimaTrocaModo = agoraMilis;
      avancarModo();
    }
  }

  // ── Deep sleep por perda de conexão BLE ───────────────
  verificarDeepSleepPorPerdaBLE(agoraMilis);

  // ── Sensor IR — ───────────────────────────────────────
  bool piscadaAtiva = (modoAtual == MODO_PISCADA || modoAtual == MODO_COMPLETO);
  if (piscadaAtiva) {
    unsigned long agoraUs = micros();
    bool tempoEntreLeituraIR = agoraUs - ultimoSampleUs >= (unsigned long)INTERVALO_US;   
    if (tempoEntreLeituraIR) {
      ultimoSampleUs = agoraUs;
      int leitura = analogRead(sensorPin);
      processarLeituraIR(leitura, conectado);
    }
  }

  // ── MPU6050 ────────────────────────────────────────────
  bool movimentoAtivo = (modoAtual == MODO_MOVIMENTO || modoAtual == MODO_COMPLETO);

  if (conectado && movimentoAtivo) {
    if (agoraMilis - ultimoMPU >= INTERVALO_MPU_MS) {
      ultimoMPU = agoraMilis;
      //Atualiza os valores das variaveis Vx, Vy
      AtualizarCoordenadasMPU();
      atualizarEstabilidade(vx, vy);
      
      //Movimento insuficiente 
      if (abs(vx) < 2) vx = 0;
      if (abs(vy) < 2) vy = 0;

      if (vx != 0 || vy != 0) {
        enviarReport(botoesAtuais, clamp127(vx), clamp127(vy));
      }
    }
  }
  delay(1);
}
