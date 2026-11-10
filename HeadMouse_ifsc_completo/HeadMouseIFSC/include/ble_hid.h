#pragma once
#include <Arduino.h>
extern bool conectado;
extern uint8_t botoesAtuais;
int8_t clamp127(int v);
void enviarReport(uint8_t botoes, int8_t x, int8_t y);
void setupBLE();
