#include <Arduino.h>
#include "../include/movimento.h"

MPU6050 mpu;
static int fator_posi;
int vx, vy;
unsigned long ultimoMPU = 0;
const int INTERVALO_MPU_MS = 10;

void AtualizarCoordenadasMPU(){
    int16_t _gx, _gy, _gz;
    mpu.getRotation(&_gx, &_gy, &_gz);
    vx = (_gx / 150)*fator_posi;
    vy = (-_gz / 150)*fator_posi;
  }
void atualizarOrientacao(float ax) {
  if (ax >=  0) fator_posi = -1;
  if (ax < 0) fator_posi = 1;
}

void setCalibrationMPU() {
  mpu.CalibrateGyro(7);
}
