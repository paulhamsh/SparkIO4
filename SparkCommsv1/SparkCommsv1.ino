#include "SparkComms.h"


byte get_preset[]{0x01,0xFE,0x00,0x00,0x53,0xFE,0x3C,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0xF0,0x01,0x09,0x01,0x02,0x01,0x00,0x00, 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0xF7};
int offset = 16;
int preset_to_get;

unsigned long t;
bool do_it;

#define BLOCK_SIZE 1000
byte block_clone[BLOCK_SIZE];
int block_clone_length;

byte block_trimmed[BLOCK_SIZE];
byte block_bits[BLOCK_SIZE];
byte block_processes[BLOCK_SIZE];

void dump_block(byte *block, int block_length) {
  Serial.print("Block from Spark - length: ");
  Serial.println(block_length);

  int lc = 16;
  for (int i = 0; i < block_length; i++) {
    byte b = block[i];
    if (b == 0xf0) {
      Serial.println();
      lc = 6;
    }
    if (b == 0x01 && block[i+1] == 0xfe) {
      lc = 16;
      Serial.println();
    }
    if (b < 16) Serial.print("0");
    Serial.print(b, HEX);
    Serial.print(" ");
    if (--lc == 0) {
      Serial.println();
      lc = 8;
    }
  }
  Serial.println();
}

void clone(byte *to, byte *from, int len) {
  memcpy(to, from, len);
}

int trim(byte *trim_blk, byte *in_blk, int in_len) {
  int new_len  = 0;
  int in_pos   = 0;
  int out_pos  = 0;
  int out_base = 0; 
  int last_sequence = -1;

  byte chk;

  byte sequence;
  byte command;
  byte sub_command;
  byte checksum;

  while (in_pos < in_len) {
    // check for 0xf7 chunnk ending
    if (in_blk[in_pos] == 0xf7) {
       in_pos++;
       trim_blk[out_base + 2] = out_pos >> 16;
       trim_blk[out_base + 3] = out_pos && 0xff;
       trim_blk[out_base + 4] += (checksum != chk);
    }    
    // check for 0x01 0xfe start of Spark 40 16-byte block header
    else if (in_blk[in_pos] == 0x01 && in_blk[in_pos + 1] == 0xfe) {
      in_pos += 16;
    }
    else if (in_blk[in_pos] == 0xf0 && in_blk[in_pos + 1] == 0x01) {
      sequence    = in_blk[in_pos + 2];
      checksum    = in_blk[in_pos + 3];
      command     = in_blk[in_pos + 4];
      sub_command = in_blk[in_pos + 5];

      out_base = out_pos;                     // move to end of last data
      out_pos = out_pos + 5;                  // save space for header
      chk = 0;
      in_pos += 6;

      if (sequence != last_sequence) {
        last_sequence = sequence;
        trim_blk[out_base]     = command;
        trim_blk[out_base + 1] = sub_command;
        trim_blk[out_base + 4] = 0;
      }
    }
    else {
      trim_blk[out_pos] = in_blk[in_pos];
      chk ^= in_blk[in_pos];
      in_pos++;
      out_pos++;
    }
  }
  return out_pos;
}


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
    // swiftly make a copy of everything and 'free' the ble block
    int len = from_spark_index;
    clone(block_clone, from_spark, len);
    dump_block(block_clone, len);
    got_spark_block = false;
    last_spark_was_bad = false;
    from_spark_index = 0;

    //dump_block(block_clone, len);
    int trim_len = trim(block_trimmed, block_clone, len);
    dump_block(block_trimmed, trim_len);
  }


}
