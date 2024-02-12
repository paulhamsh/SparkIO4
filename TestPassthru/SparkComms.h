#ifndef SparkComms_h
#define SparkComms_h

#define SPARK_BT_NAME  "Spark 40"
#define SPARK_BLE_NAME  "Spark 40 BLE"

#define DEBUG(x) Serial.println(x);

#ifdef CLASSIC
#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#else
#include "NimBLEDevice.h"
#endif

bool ble_passthru;
bool ble_passthru_to_app_ready;
bool ble_passthru_to_spark_ready;

byte to_app[5000];
int to_app_index = 0;
byte to_spark[5000];
int to_spark_index = 0;


#define BLE_BUFSIZE 5000

#define C_SERVICE "ffc0"
#define C_CHAR1   "ffc1"
#define C_CHAR2   "ffc2"

#define S_SERVICE "ffc0"
#define S_CHAR1   "ffc1"
#define S_CHAR2   "ffc2"

#define MAX_SCAN_COUNT 2

bool connect_to_all();
void connect_spark();

bool sp_available();
bool app_available();
uint8_t sp_read();
uint8_t app_read();
void sp_write(byte *buf, int len);

bool ble_app_connected;

bool connected_sp;
bool found_sp;

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic_receive;
BLECharacteristic *pCharacteristic_send;

BLEAdvertising *pAdvertising;

BLEScan *pScan;
BLEScanResults pResults;
BLEAdvertisedDevice device;

BLEClient *pClient_sp;
BLERemoteService *pService_sp;
BLERemoteCharacteristic *pReceiver_sp;
BLERemoteCharacteristic *pSender_sp;
BLERemoteDescriptor* p2902_sp;
BLEAdvertisedDevice *sp_device;

#endif
