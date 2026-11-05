#include <Arduino.h>
#include "modos.h"

Modo modoAtual = MODO_COMPLETO;
const char* nomeModo[] = { "", "SO MOVIMENTO", "SO PISCADA", "COMPLETO" };
unsigned long ultimoPressBoot = 0;

void avancarModo() {
  modoAtual = (Modo)(modoAtual % 3 + 1);
  //Serial.print(">>> MODO: ");
  //Serial.println(nomeModo[modoAtual]);
}
