#include <Arduino.h>
#include "../include/energia.h"
#include <esp_wifi.h>
#include <esp32-hal-cpu.h>
#include "../include/ble_hid.h"
#define TEMPO_SEM_BT_MS 60000
unsigned long tempoSemConexao = 0;
static bool btConectadoAntes = false;

void configurarEnergia() {
  esp_wifi_stop();
  esp_wifi_deinit();
  setCpuFrequencyMhz(50);
}

void verificarDeepSleepPorPerdaBLE(unsigned long agoraMilis) {
  if (conectado) {
    tempoSemConexao  = agoraMilis;
    btConectadoAntes = true;
  } else if (btConectadoAntes && (agoraMilis - tempoSemConexao > TEMPO_SEM_BT_MS)) {
    Serial.println("Sem BLE por 60s - deep sleep de 10s");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(10ULL * 1000000);
    esp_deep_sleep_start();
  }
}
