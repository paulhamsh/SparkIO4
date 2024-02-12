#include "SparkComms.h"

const uint8_t notifyOn[] = {0x1, 0x0};

// client callback for connection to Spark

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    DEBUG("callback: Spark connected");
  }

  void onDisconnect(BLEClient *pclient)
  {
//    if (pclient->isConnected()) {
    connected_sp = false;         
    DEBUG("callback: Spark disconnected");   
  }
};

// server callback for connection to BLE app

class MyServerCallback : public BLEServerCallbacks
{
  void onConnect(BLEServer *pserver)
  {
     if (pserver->getConnectedCount() == 1) {
      DEBUG("callback: BLE app connection event and is connected"); 
    }
    else {
      DEBUG("callback: BLE app connection event and is not really connected");   
    }
  }

  void onDisconnect(BLEServer *pserver)
  {
//    if (pserver->getConnectedCount() == 1) {
    DEBUG("callback: BLE app disconnected");
  }
};


// Blocks sent to the app are max length 0xad or 173 bytes. This includes the 16 byte block header.
// Without that header the block is 157 bytes.
// For Spark 40 BLE they are sent as 100 bytes then 73 bytes.
// 
// From the amp, max size is 0x6a or 106 bytes (90 bytes plus 16 byte block header).
// For the Spark 40 these are sent in chunk of 106 bytes.
// MINI and GO do not use the block header so just transmit 90 bytes.
// From the MINI and GO, they are sent in chunks of 20 bytes up to the 90 bytes (so 20 + 20 + 20 + 20 + 10). 
// 
//
// Chunks sent from the amp have max size of 39 bytes, 0x27.
// This is 6 byte header, 1 byte footer, 4 data chunks of 7 bytes, 4 '8 bit' bytes. So 6 + 1 +32 = 39.
// Because of the multi-chunk header of 3 bytes, there are 32 - 3 = 29 data bytes (0x19)
//
// Example
// F0 01 04 1F 03 01   20  0E 00 19  00 00 59 24   00 39 44 32 46 32 41 41   00 33 2D 34 45 43 35 2D   00 34 42 44 37 2D 41 33   F7


void notifyCB_sp(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  int i;
  byte b;

  Serial.print("Got from spark:        ");
  Serial.print(length);
  Serial.print(":");
  b = pData[length-1];
  if (b < 16) Serial.print("0");
  Serial.print(b, HEX);    
  Serial.println();

  for (i = 0; i < length; i++) {
    b = pData[i];
    //ble_in.add(b);
    to_app[to_app_index++] = b;
  /*
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);    
    Serial.print(" ");
  */
  }
  //Serial.println();

  if (to_app_index == 20 || to_app_index == 40 || to_app_index == 60 || to_app_index == 80 )
    Serial.println("Got a partial block");
  else {
    // Could be 90, 106 or just another number not aligned as above
    // But what about a final chunk of, say, 20 bytes? Must check for timeout or an f7
    Serial.println("I think end of a block");
    ble_passthru_to_app_ready = true;
  }
  //ble_in.commit();
}


class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    int i = 0;
    byte b;
    for (auto & it : pCharacteristic->getValue()) {
      //ble_app_in.add(it);
      i++;
      b = it;
      /*
      if (b < 16) Serial.print("0");
      Serial.print(b, HEX);    
      Serial.print(" ");
      if (i % 20 == 0) Serial.println();
      */

      to_spark[to_spark_index++] = b;

    }


    Serial.print("Got from app:          ");
    Serial.print(i);
    Serial.print(":");
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);    
    Serial.println();

    // For MINI and GO will be 100 then 73 for a block of 173
    // For Spark 40 this xxxxxxxxxxxxxxxxxx

    if (to_spark_index == 100)
      Serial.println("Got a partial block");
    else {
      // Mostly going to be 73 or another value - but a partial block is always 100
      Serial.println("I think end of a block");
      ble_passthru_to_spark_ready = true;
    }

    //ble_app_in.commit();
  };
};

static CharacteristicCallbacks chrCallbacks_s, chrCallbacks_r;

BLEUUID SpServiceUuid(C_SERVICE);

void connect_spark() {
  if (found_sp && !connected_sp) {
    if (pClient_sp != nullptr && pClient_sp->isConnected())
       DEBUG("HMMMM - connect_spark() SAYS I WAS CONNECTED ANYWAY");
    
    if (pClient_sp->connect(sp_device)) {
#if defined CLASSIC  && !defined HELTEC_WIFI
      pClient_sp->setMTU(517);  
#endif

      Serial.print("GetMTU ");
      Serial.println(pClient_sp->getMTU());

      connected_sp = true;
      pService_sp = pClient_sp->getService(SpServiceUuid);
      if (pService_sp != nullptr) {
        pSender_sp   = pService_sp->getCharacteristic(C_CHAR1);
        pReceiver_sp = pService_sp->getCharacteristic(C_CHAR2);
        if (pReceiver_sp && pReceiver_sp->canNotify()) {
#ifdef CLASSIC
          pReceiver_sp->registerForNotify(notifyCB_sp);
          p2902_sp = pReceiver_sp->getDescriptor(BLEUUID((uint16_t)0x2902));
          if (p2902_sp != nullptr)
             p2902_sp->writeValue((uint8_t*)notifyOn, 2, true);
#else
          if (!pReceiver_sp->subscribe(true, notifyCB_sp, true)) {
            connected_sp = false;
            DEBUG("Spark disconnected");
            NimBLEDevice::deleteClient(pClient_sp);
          }   
#endif
        } 
      }
      DEBUG("connect_spark(): Spark connected");
    }
  }
}


bool connect_to_all() {
  int i, j;
  int counts;
  uint8_t b;

  BLEDevice::init(SPARK_BLE_NAME);
  BLEDevice::setMTU(517);
  pClient_sp = BLEDevice::createClient();
  pClient_sp->setClientCallbacks(new MyClientCallback());
 
  pScan = BLEDevice::getScan();

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallback());  
  pService = pServer->createService(S_SERVICE);

#ifdef CLASSIC  
  pCharacteristic_receive = pService->createCharacteristic(S_CHAR1, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic_send = pService->createCharacteristic(S_CHAR2, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
#else
  pCharacteristic_receive = pService->createCharacteristic(S_CHAR1, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pCharacteristic_send = pService->createCharacteristic(S_CHAR2, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY); 
#endif

  pCharacteristic_receive->setCallbacks(&chrCallbacks_r);
  pCharacteristic_send->setCallbacks(&chrCallbacks_s);
#ifdef CLASSIC
  pCharacteristic_send->addDescriptor(new BLE2902());
#endif

  pService->start();

#ifndef CLASSIC
  pServer->start(); 
#endif

  pAdvertising = BLEDevice::getAdvertising(); // create advertising instance
  pAdvertising->addServiceUUID(pService->getUUID()); // tell advertising the UUID of our service
  pAdvertising->setScanResponse(true);  

  // Connect to Spark
  connected_sp = false;
  found_sp = false;

  DEBUG("Scanning...");

  counts = 0;
  while (!found_sp && counts < MAX_SCAN_COUNT) {   // assume we only use a pedal if on already and hopefully found at same time as Spark, don't wait for it
    counts++;
    pResults = pScan->start(4);
    
    for(i = 0; i < pResults.getCount()  && !found_sp; i++) {
      device = pResults.getDevice(i);

      if (device.isAdvertisingService(SpServiceUuid)) {
        DEBUG("Found Spark");
        found_sp = true;
        connected_sp = false;
        sp_device = new BLEAdvertisedDevice(device);
      }
      
    }
  }

  if (!found_sp) return false;   // failed to find the Spark within the number of counts allowed (MAX_SCAN_COUNT)
  
    // Set up client
  connect_spark();

  DEBUG("Available for app to connect...");  
  pAdvertising->start(); 
  return true;
}
