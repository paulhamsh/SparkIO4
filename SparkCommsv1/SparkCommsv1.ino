#include "SparkComms.h"


byte get_preset[]{0x01,0xFE,0x00,0x00,0x53,0xFE,0x3C,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0xF0,0x01,0x09,0x01,0x02,0x01,0x00,0x00, 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xF7};
int offset = 16;
int preset_to_get;

unsigned long t;
bool do_it;


void setup() {
  Serial.begin(115200);
  connect_to_all();
  DEBUG("Spark found and connected - starting");

  ble_passthru = true;

  t = millis();
  do_it = false;
  preset_to_get = 0;

}


void loop() {
  // pre-wait before starting to request presets
  if (millis() - t > 20000 && !do_it) {
    do_it = true;
    t = millis();
  };

  // request presets in order
  if (millis() - t > 5000 && do_it) {
    Serial.println("Sending preset request");
    get_preset[offset + 2] = 0x30; // sequence number
    get_preset[offset + 3] = preset_to_get; //checksum
    get_preset[offset + 8] = preset_to_get;

    pSender_sp->writeValue(get_preset, sizeof(get_preset), false);
    t = millis();
    preset_to_get++;
    if (preset_to_get > 3) preset_to_get = 0;
  };

  if (last_spark_was_bad) {
    Serial.println("Was a bad block");
    last_spark_was_bad = false;
  }

  if (got_spark_block) {
    Serial.print("Block from Spark:  length: ");
    Serial.println(from_spark_index);

    for (int i = 0; i < from_spark_index; i++) {
      byte b = from_spark[i];
      if (b < 16) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
      if (i % 32 == 31) Serial.println();
    }
    Serial.println();

    got_spark_block = false;
    last_spark_was_bad = false;
    from_spark_index = 0;
  }


}
