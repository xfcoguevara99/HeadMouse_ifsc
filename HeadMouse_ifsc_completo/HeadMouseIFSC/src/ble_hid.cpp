#include <Arduino.h>
#include "../include/ble_hid.h"
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <HIDTypes.h>

static BLEHIDDevice* hid;
static BLECharacteristic* inputMouse;
static BLEServer* pServer;
bool conectado = false;
uint8_t botoesAtuais = 0;

static const uint8_t reportMap[] = {
  0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
  0x85, 0x01, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00,
  0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01,
  0x75, 0x05, 0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
  0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03,
  0x81, 0x06, 0xC0, 0xC0
};

class MinhasServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    conectado = true;
    Serial.println("Conectado!");
  }
  void onDisconnect(BLEServer* server) {
    conectado = false;
    Serial.println("Desconectado! Reiniciando advertising...");
    delay(200);
    pServer->getAdvertising()->start();
  }
};

int8_t clamp127(int v) {
  if (v > 127) return 127;
  if (v < -127) return -127;
  return (int8_t)v;
}

void enviarReport(uint8_t botoes, int8_t x, int8_t y) {
  if (!conectado) return;
  uint8_t report[4] = { botoes, (uint8_t)x, (uint8_t)y, 0 };
  inputMouse->setValue(report, 4);
  inputMouse->notify();
}

void setupBLE() {
  BLEDevice::init("HeadMouseIFSC-2026");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MinhasServerCallbacks());

  hid = new BLEHIDDevice(pServer);
  inputMouse = hid->inputReport(1);

  hid->manufacturer()->setValue("DIY");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x01);

  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->startServices();

  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();

  hid->setBatteryLevel(100);
}
