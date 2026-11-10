#pragma once
enum Modo { MODO_MOVIMENTO = 1, MODO_PISCADA = 2, MODO_COMPLETO = 3 };
extern Modo modoAtual;
extern const char* nomeModo[];
extern unsigned long ultimaTrocaModo;

void avancarModo();
