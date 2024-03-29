
//#define CLASSIC

#include "SparkComms.h"

// Board specific #includes



void setup() {
  Serial.begin(115200);
  connect_to_all();
  DEBUG("Spark found and connected - starting");
  Serial.println("------------------------------------");
 
  ble_passthru_to_app_ready = false;
  ble_passthru_to_spark_ready = false;
  ble_passthru = true;

}

void loop() {

  byte b;

  if (ble_passthru_to_app_ready) {
    pCharacteristic_send->setValue(to_app, to_app_index);
    pCharacteristic_send->notify(true);
     Serial.print("Passthru to app:   ");
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

    Serial.print("Passthru to spark: ");
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
