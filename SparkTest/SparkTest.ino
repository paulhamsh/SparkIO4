// PRogram to send preset requests to a Spark and check what is received, implementing timeouts

#include <NimBLEDevice.h>

#define SERVICE       "FFC0"
#define CHAR_SEND     "FFC1"
#define CHAR_RECEIVE  "FFC2"

static NimBLEAdvertisedDevice* advDevice;

NimBLERemoteService* pSvc = nullptr;
NimBLERemoteCharacteristic* pChrSend = nullptr;
NimBLERemoteCharacteristic* pChrReceive = nullptr;
NimBLERemoteDescriptor* pDsc = nullptr;

NimBLEClient* pClient = nullptr;

static bool doConnect = false;
static uint32_t scanTime = 0; /** 0 = scan forever */

#define BUF_SIZE 5000
byte buf[BUF_SIZE];
int buf_index = 0;
int last_len = 0;

class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.print("Advertised Device found: ");
        //Serial.println(advertisedDevice->toString().c_str());
        Serial.print(advertisedDevice->getAddress().toString().c_str());
        Serial.print("   ");
        Serial.println(advertisedDevice->getName().c_str());

        if(advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE)))
        {
            Serial.println("Found Spark");
            NimBLEDevice::getScan()->stop();
            advDevice = advertisedDevice;
            doConnect = true;
        }
    };
};


unsigned long last_callback_time;
bool got_packet;

void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* chunk_data, size_t chunk_length, bool isNotify){

    //Serial.print("Len = ");
    //Serial.print(length);
    //Serial.print(":   ");
    //if (pData[length-1] < 16) Serial.print("0");
    //Serial.println(pData[length-1], HEX);

    //last_len = length;
    last_callback_time = millis();

    for (int i = 0; i < chunk_length; i++) {
      buf[buf_index++] = chunk_data[i];
      //if (pData[i] < 16) Serial.print("0");
      //Serial.print(pData[i], HEX);
      //Serial.print(" ");

    }
    Serial.print(chunk_length);
    Serial.print(":");
    int b = buf[buf_index-1];
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);    
    Serial.print(" ");

    if (chunk_length != 20 && chunk_length != 10 && chunk_length != 106 && buf[buf_index-1] == 0xf7) got_packet = true;
}

void scanEndedCB(NimBLEScanResults results){
    Serial.println("Scan Ended");
}



bool connectToServer() {
    if(NimBLEDevice::getClientListSize()) {
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if(pClient){
            if(!pClient->connect(advDevice, false)) {
                Serial.println("Reconnect failed");
                return false;
            }
            Serial.println("Reconnected client");
        }
        else {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    if(!pClient) {
        if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }
        pClient = NimBLEDevice::createClient();

        if (!pClient->connect(advDevice)) {
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Failed to connect");
            return false;
        }
    }

    if(!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Serial.println("Failed to connect");
            return false;
        }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());

    pSvc = pClient->getService(SERVICE);
    if(pSvc) { 
        pChrReceive = pSvc->getCharacteristic(CHAR_RECEIVE);
        pChrSend = pSvc->getCharacteristic(CHAR_SEND);

        if(pChrReceive) { 
            if (pChrReceive->canNotify()) {
                if(!pChrReceive->subscribe(true, notifyCB, true)) {
                    pClient->disconnect();
                    Serial.println("Couldn't subscribe");
                    return false;
                }
            }
        }
    } 

    //pClient->setMTU(BLE_ATT_MTU_MAX);
    return true;
}

void setup (){
    Serial.begin(115200);
    Serial.println("Starting...");
    NimBLEDevice::init("");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(scanTime, scanEndedCB);

    while(!doConnect){
        delay(1);
    }

    if(connectToServer()) {
        Serial.println("Connect success");
    } 

    got_packet = false;
    delay(2000);
}


byte get_preset[]{0x01,0xFE,0x00,0x00,0x53,0xFE,0x3C,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0xF0,0x01,0x09,0x01,0x02,0x01,0x00,0x00, 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xF7};
int offset = 16;

/*
byte get_preset[]{0xF0,0x01,0x09,0x01,0x02,0x01,0x00,0x00, 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xF7};                 
int offset = 0;
*/
/*
byte get_preset[]{0xf0,0x01,0x09,0x01,0x02,0x01,0x00,0x00, 0x01,0xf7};
int offset = 0;
*/

//  0xf0 0x01  {sequence} {checksum} {command} {sub-command} {data} {0xf7}
uint8_t preset_to_get = 0;
int counter = 0;
uint8_t sequence = 20;
#define CB_TIMEOUT 200
#define NEXT_SEND_DELAY 3000

unsigned long next_send = 0;

void loop () {
    // send every so often 
    if (millis() >= next_send) {
      get_preset[offset + 2] = sequence; // sequence number
      sequence++;
      if (sequence > 0x5f) sequence = 0;

      get_preset[offset + 3] = preset_to_get; //checksum
      get_preset[offset + 8] = preset_to_get; //preset number
    
      Serial.print("Asking for ");
      Serial.print(preset_to_get);
      Serial.print(" :: ");

      if(pChrSend->canWrite()) {
        pChrSend->writeValue(get_preset, sizeof(get_preset));
      }   

      next_send = millis() + NEXT_SEND_DELAY;
    } 


    if (buf_index > 0) {
      
      if (got_packet) {
        Serial.println();
        Serial.println(">> Got a packet ");
        got_packet = false;
        buf_index = 0;
        counter++;
        if (counter >= 5) {
          counter = 0;
          preset_to_get++;
            if (preset_to_get > 3) preset_to_get = 0;
        }
      }
      else if (millis() - last_callback_time > CB_TIMEOUT) {
        if (buf[buf_index-1] == 0xf7) {
          Serial.println();
          Serial.println(">> Time out, but f7 at end of buffer");
          buf_index = 0;
        }
        else {
          Serial.println();
          Serial.println(">> Timed out");
          buf_index = 0;
        }
      }
 

    }
}
