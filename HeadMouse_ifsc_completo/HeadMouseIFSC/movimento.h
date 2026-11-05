#pragma once
#include <MPU6050.h>
extern MPU6050 mpu;
extern int vx, vy;
extern unsigned long ultimoMPU;
void AtualizarCoordenadasMPU();
void setCalibrationMPU();
void atualizarOrientacao(float ax);
