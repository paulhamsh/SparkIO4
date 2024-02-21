#include "SparkComms.h"
#include "SparkIO.h"
#include "RingBuffer.h"
#include "SparkStructures.h"
#include "testdata.h"

byte get_preset[]{0x01, 0xFE, 0x00, 0x00, 0x53, 0xFE, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                  0xF0, 0x01, 0x09, 0x01, 0x02, 0x01,
                  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00 ,0x00, 0x00, 0x00, 0x00,
                  0xF7};
int offset = 16;
int preset_to_get;

unsigned long t;
bool do_it;


byte block_app[BLOCK_SIZE];


void setup() {
  Serial.begin(115200);
  connect_to_all();
  DEBUG("Starting");

  ble_passthru = true;

  t = millis();
  do_it = false;
  preset_to_get = 0;

}


unsigned int cmdsub;
SparkMessage message;
SparkPreset preset;

void loop() {
  int len;
  int trim_len;

  // pre-wait before starting to request presets
  if (millis() - t > 20000 && !do_it) {
    do_it = true;
    t = millis();
  };


  spark_process();
  app_process();

  if (spark_message_in.get_message(&cmdsub, &message, &preset)) {
    Serial.print("SPK: ");
    Serial.println(cmdsub, HEX);
  };


  if (app_message_in.get_message(&cmdsub, &message, &preset)) {
    Serial.print("APP: ");
    Serial.println(cmdsub, HEX);
  };

  /*
  if (last_app_was_bad) {
    Serial.println("App sent a bad block");
    last_app_was_bad = false;
  }
  */

/*  
  if (got_app_block) {
    len = from_app_index;
    clone(block_app, from_app, len);
    //dump_raw_block(block_app, len);
    got_app_block = false;
    last_app_was_bad = false;
    from_app_index = 0;
    dump_raw_block(block_app, len);
  }
  */
   

}
