#pragma once
extern unsigned long ultimoSampleUs;
void resetarLeiturasIR();                       // o memset do setup()
void processarLeituraIR(int leitura, bool btConectado);
