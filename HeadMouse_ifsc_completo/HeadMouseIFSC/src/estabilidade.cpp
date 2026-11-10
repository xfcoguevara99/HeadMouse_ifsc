#include <Arduino.h>
#include "../include/estabilidade.h"
#include "../include/config.h""

#define JANELA_ESTABILIDADE_MS  300   // Janela de observação
#define LIMITE_MOVIMENTO     60    // Movimento máximo permitido na janela
#define TEMPO_MINIMO_ESTAVEL_MS 100   // Tempo mínimo consecutivo estável
#define NUM_CICLOS_ESTABILIDADE (JANELA_ESTABILIDADE_MS / INTERVALO_MPU_MS) // 300/10 = 30

static int bufferMovimento[NUM_CICLOS_ESTABILIDADE];
static int indexMovimento        = 0;
static unsigned long tempoEstavelInicio = 0;  // Quando o cursor começou a ficar estável
int acumuladoMovimento    = 0;         // Soma corrente — atualizada incrementalmente
bool cursorEstavel        = false;

void atualizarJanelaMovimento(int vx, int vy) {
  int acumulador = abs(vx) + abs(vy);
  // Atualização incremental: subtrai o valor mais antigo, soma o novo
  // Evita recalcular a soma inteira a cada ciclo
  acumuladoMovimento -= bufferMovimento[indexMovimento];
  acumuladoMovimento += acumulador;
  // Guarda o novo valor no buffer circular
  bufferMovimento[indexMovimento] = acumulador;
  indexMovimento = (indexMovimento + 1) % NUM_CICLOS_ESTABILIDADE;
}

static bool movimentoAbaixoDoLimite() {
  return (acumuladoMovimento < LIMITE_MOVIMENTO);
}

static void atualizarEstadoEstavel(bool abaixoDoLimite) {
  unsigned long agora = millis();
  if (abaixoDoLimite) {
    if (!cursorEstavel) {
      // Acabou de entrar no estado estável — registra o início
      tempoEstavelInicio = agora;
      cursorEstavel = true;
    }
    // cursorEstavel permanece true enquanto acumulado < threshold
  } else {
    // Movimento detectado — reseta tudo
    cursorEstavel = false;
    tempoEstavelInicio = agora;
  }
}

void atualizarEstabilidade(int vx, int vy) {
  atualizarJanelaMovimento(vx, vy);
  bool abaixoDoLimite = movimentoAbaixoDoLimite();
  atualizarEstadoEstavel(abaixoDoLimite);
}

bool cursorEstavelConfirmado() {
  if (!cursorEstavel) return false;
  return (millis() - tempoEstavelInicio >= TEMPO_MINIMO_ESTAVEL_MS);
}
