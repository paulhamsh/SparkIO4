
#define ESP_DEVKIT
//#define HELTEC_WIFI
//#define M5CORE
//#define M5CORE2
//#define M5STICK

//#define CLASSIC

#include "SparkComms.h"

// Board specific #includes

#if defined HELTEC_WIFI
  #include "heltec.h"
#elif defined M5STICK
  #include <M5StickC.h>
#elif defined M5CORE2
  #include <M5Core2.h>
#elif defined M5CORE
  #include <M5Stack.h>
#endif


void setup() {
#if defined HELTEC_WIFI
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
#elif defined M5STICK || defined M5CORE2 || defined M5CORE
  M5.begin();
#endif
#if defined M5CORE
  M5.Power.begin();
#endif

  Serial.begin(115200);
  connect_to_all();
  DEBUG("Spark found and connected - starting");
 
  ble_passthru_to_app_ready = false;
  ble_passthru_to_spark_ready = false;
  ble_passthru = true;

}

void loop() {

  byte b;

#if defined M5STICK || defined M5CORE2 || defined M5CORE
  M5.update();
#endif


  if (ble_passthru_to_app_ready) {
    pCharacteristic_send->setValue(to_app, to_app_index);
    pCharacteristic_send->notify(true);
     Serial.print("Passthru to app:       ");
    Serial.print(to_app_index);
    Serial.print(":");
    int b = to_app[to_app_index-1];
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);    
    Serial.println();

    to_app_index = 0;
    ble_passthru_to_app_ready = false;
  }

  if (ble_passthru_to_spark_ready) {
    pSender_sp->writeValue(to_spark, to_spark_index, false);

    Serial.print("Passthru to spark:     ");
    Serial.print(to_spark_index);
    Serial.print(":");
    int b = to_spark[to_spark_index-1];
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);    
    Serial.println();

    to_spark_index = 0;
    ble_passthru_to_spark_ready = false;
  }
}
