#pragma once
#include <Arduino.h>
extern unsigned long tempoSemConexao;   // o setup() inicializa
void configurarEnergia();
void verificarDeepSleepPorPerdaBLE(unsigned long agoraMilis);
