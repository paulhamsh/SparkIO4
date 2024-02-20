#include "SparkIO.h"

/*  SparkIO
 *  
 *  SparkIO handles communication to and from the Positive Grid Spark amp over bluetooth for ESP32 boards
 *  
 *  From the programmers perspective, you create and read two formats - a Spark Message or a Spark Preset.
 *  The Preset has all the data for a full preset (amps, effects, values) and can be sent or received from the amp.
 *  The Message handles all other changes - change amp, change effect, change value of an effect parameter, change hardware preset and so on
 *  
 *  The class is initialized by creating an instance such as:
 *  
 *  SparkClass sp;
 *  
 *  Conection is handled with the two commands:
 *  
 *    sp.start_bt();
 *    sp.connect_to_spark();
 *    
 *  The first starts up bluetooth, the second connects to the amp
 *  
 *  
 *  Messages and presets to and from the amp are then queued and processed.
 *  The essential thing is the have the process() function somewhere in loop() - this handles all the processing of the input and output queues
 *  
 *  loop() {
 *    ...
 *    sp.process()
 *    ...
 *    do something
 *    ...
 *  }
 * 
 * Sending functions:
 *     void create_preset(SparkPreset *preset);    
 *     void get_serial();    
 *     void turn_effect_onoff(char *pedal, bool onoff);    
 *     void change_hardware_preset(uint8_t preset_num);    
 *     void change_effect(char *pedal1, char *pedal2);    
 *     void change_effect_parameter(char *pedal, int param, float val);
 *     
 *     These all create a message or preset to be sent to the amp when they reach the front of the 'send' queue
 *  
 * Receiving functions:
 *     bool get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
 * 
 *     This receives the front of the 'received' queue - if there is nothing it returns false
 *     
 *     Based on whatever was in the queue, it will populate fields of the msg parameter or the preset parameter.
 *     Eveything apart from a full preset sent from the amp will be a message.
 *     
 *     You can determine which by inspecting cmdsub - this will be 0x0301 for a preset.
 *     
 *     Other values are:
 *     
 *     cmdsub       str1                   str2              val           param1             param2                onoff
 *     0323         amp serial #
 *     0337         effect name                              effect val    effect number
 *     0306         old effect             new effect
 *     0338                                                                0                  new hw preset (0-3)
 * 
 * 
 * 
 */


void dump_buf(char *hdr, uint8_t *buf, int len) {
 
  Serial.print(hdr);
  Serial.print(" <");
  Serial.print(buf[18], HEX);
  Serial.print("> ");
  Serial.print(buf[len-2], HEX);
  Serial.print(" ");
  Serial.print(buf[len-1], HEX);  
  
  int i;
  for (i = 0; i < len ; i++) {
    if (buf[i]<16) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
    if (i % 16 == 15) Serial.println();
  };
  Serial.println(); 
}

// UTILITY FUNCTIONS

void uint_to_bytes(unsigned int i, uint8_t *h, uint8_t *l) {
  *h = (i & 0xff00) / 256;
  *l = i & 0xff;
}

void bytes_to_uint(uint8_t h, uint8_t l,unsigned int *i) {
  *i = (h & 0xff) * 256 + (l & 0xff);
}

 
// MAIN SparkIO CLASS

uint8_t chunk_header_from_spark[16]{0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t chunk_header_to_spark[16]  {0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void spark_start(bool passthru) {
}

// 
// Main processing routine
//

void spark_process() 
{
}


void app_process() 
{
}


// MESSAGE INPUT ROUTINES
// Routine to read messages from the in_message ring buffer and copy to a message C structurer

void MessageIn::set_from_array(uint8_t *in, int size) {
  in_message.set_from_array(in, size);
}

void MessageIn::read_byte(uint8_t *b)
{
  uint8_t a;
  in_message.get(&a);
  *b = a;
}   

void MessageIn::read_uint(uint8_t *b)
{
  uint8_t a;
  in_message.get(&a);
  if (a == 0xCC)
    in_message.get(&a);
  *b = a;
}
   
void MessageIn::read_string(char *str)
{
  uint8_t a, len;
  int i;

  read_byte(&a);
  if (a == 0xd9) {
    read_byte(&len);
  }
  else if (a >= 0xa0) {
    len = a - 0xa0;
  }
  else {
    read_byte(&a);
    if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
    len = a - 0xa0;
  }

  if (len > 0) {
    // process whole string but cap it at STR_LEN-1
    for (i = 0; i < len; i++) {
      read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void MessageIn::read_prefixed_string(char *str)
{
  uint8_t a, len;
  int i;

  read_byte(&a); 
  read_byte(&a);

  if (a < 0xa0 || a >= 0xc0) DEBUG("Bad string");
  len = a-0xa0;

  if (len > 0) {
    for (i = 0; i < len; i++) {
      read_byte(&a);
      if (a<0x20 || a>0x7e) a=0x20; // make sure it is in ASCII range - to cope with get_serial 
      if (i < STR_LEN -1) str[i]=a;
    }
    str[i > STR_LEN-1 ? STR_LEN-1 : i]='\0';
  }
  else {
    str[0]='\0';
  }
}   

void MessageIn::read_float(float *f)
{
  union {
    float val;
    byte b[4];
  } conv;   
  uint8_t a;
  int i;

  read_byte(&a);  // should be 0xca
  if (a != 0xca) return;

  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  for (i=3; i>=0; i--) {
    read_byte(&a);
    conv.b[i] = a;
  } 
  *f = conv.val;
}

void MessageIn::read_onoff(bool *b)
{
  uint8_t a;
   
  read_byte(&a);
  if (a == 0xc3)
    *b = true;
  else // 0xc2
    *b = false;
}

// The functions to get the message

bool MessageIn::get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset)
{
  uint8_t cmd, sub, len_h, len_l;
  uint8_t sequence;
  uint8_t chksum_errors;

  unsigned int len;
  unsigned int cs;
   
  uint8_t junk;
  int i, j;
  uint8_t num;

  if (in_message.is_empty()) return false;
  
  read_byte(&cmd);
  read_byte(&sub);
  read_byte(&len_h);
  read_byte(&len_l);
  read_byte(&chksum_errors);
  read_byte(&sequence);

  bytes_to_uint(len_h, len_l, &len);
  bytes_to_uint(cmd, sub, &cs);

  *cmdsub = cs;
  switch (cs) {

    // 0x02 series - requests
    // get preset information
    case 0x0201:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      for (i=0; i < 30; i++) read_byte(&junk); // 30 bytes of 0x00
      break;            
    // get current hardware preset number - this is a request with no payload
    case 0x0210:
      break;
    // get amp name - no payload
    case 0x0211:
      break;
    // get name - this is a request with no payload
    case 0x0221:
      break;
    // get serial number - this is a request with no payload
    case 0x0223:
      break;
    // the UNKNOWN command - 0x0224 00 01 02 03
    case 0x0224:
    case 0x022a:
    case 0x032a:
      // the data is a fixed array of four bytes (0x94 00 01 02 03)
      read_byte(&junk);
      read_uint(&msg->param1);
      read_uint(&msg->param2);
      read_uint(&msg->param3);
      read_uint(&msg->param4);
    // get firmware version - this is a request with no payload
    case 0x022f:
      break;
    // change effect parameter
    case 0x0104:
      read_string(msg->str1);
      read_byte(&msg->param1);
      read_float(&msg->val);
      break;
    // change of effect model
    case 0x0306:
    case 0x0106:
      read_string(msg->str1);
      read_string(msg->str2);
      break;
    // get current hardware preset number
    case 0x0310:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // get name
    case 0x0311:
      read_string(msg->str1);
      break;
    // enable / disable an effect
    // and 0x0128 amp info command
    case 0x0315:
    case 0x0115:
    case 0x0128:
      read_string(msg->str1);
      read_onoff(&msg->onoff);
      break;
    // get serial number
    case 0x0323:
      read_string(msg->str1);
      break;
    // store into hardware preset
    case 0x0327:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // amp info   
    case 0x0328:
      read_float(&msg->val);
      break;  
    // firmware version number
    case 0x032f:
      // really this is a uint32 but it is just as easy to read into 4 uint8 - a bit of a cheat
      read_byte(&junk);           // this will be 0xce for a uint32
      read_byte(&msg->param1);      
      read_byte(&msg->param2); 
      read_byte(&msg->param3); 
      read_byte(&msg->param4); 
      break;
    // change of effect parameter
    case 0x0337:
      read_string(msg->str1);
      read_byte(&msg->param1);
      read_float(&msg->val);
      break;
    // tuner
    case 0x0364:
      read_byte(&msg->param1);
      read_float(&msg->val);
      break;
    case 0x0365:
      read_onoff(&msg->onoff);
      break;
    // change of preset number selected on the amp via the buttons
    case 0x0338:
    case 0x0138:
      read_byte(&msg->param1);
      read_byte(&msg->param2);
      break;
    // license key
    case 0x0170:
      for (i = 0; i < 64; i++)
        read_uint(&license_key[i]);
      break;
    // response to a request for a full preset
    case 0x0301:
    case 0x0101:
      read_byte(&preset->curr_preset);
      read_byte(&preset->preset_num);
      read_string(preset->UUID); 
      read_string(preset->Name);
      read_string(preset->Version);
      read_string(preset->Description);
      read_string(preset->Icon);
      read_float(&preset->BPM);

      for (j=0; j<7; j++) {
        read_string(preset->effects[j].EffectName);
        read_onoff(&preset->effects[j].OnOff);
        read_byte(&num);
        preset->effects[j].NumParameters = num - 0x90;
        for (i = 0; i < preset->effects[j].NumParameters; i++) {
          read_byte(&junk);
          read_byte(&junk);
          read_float(&preset->effects[j].Parameters[i]);
        }
      }
      read_byte(&preset->chksum);  

      break;
    // tap tempo!
    case 0x0363:
      read_float(&msg->val);  
      break;
    case 0x0470:
    case 0x0428:
      read_byte(&junk);
      break;
    // acks - no payload to read - no ack sent for an 0x0104
    case 0x0401:
    case 0x0501:
    case 0x0406:
    case 0x0415:
    case 0x0438:
    case 0x0465:
//      Serial.print("Got an ack ");
//      Serial.println(cs, HEX);
      break;
    default:
      Serial.print("Unprocessed message SparkIO ");
      Serial.print (cs, HEX);
      Serial.print(":");
      for (i = 0; i < len - 4; i++) {
        read_byte(&junk);
        Serial.print(junk, HEX);
        Serial.print(" ");
      }
      Serial.println();
  }
  return true;
}

//
// Output routines
//

void MessageOut::start_message(int cmdsub)
{
  int om_cmd, om_sub;
  
  om_cmd = (cmdsub & 0xff00) >> 8;
  om_sub = cmdsub & 0xff;

  out_message.add(om_cmd);
  out_message.add(om_sub);
  out_message.add(0);      // placeholder for length
  out_message.add(0);      // placeholder for length
  out_message.add(0);      // placeholder for checksum errors
  out_message.add(0);      // placeholder for sequence number

  out_msg_chksum = 0;
}

void MessageOut::end_message()
{
  unsigned int len;
  uint8_t len_h, len_l;
  
  len = out_message.get_len();
  uint_to_bytes(len, &len_h, &len_l);
  
  out_message.set_at_index(2, len_h);   
  out_message.set_at_index(3, len_l);
  out_message.commit();
}

void MessageOut::write_byte_no_chksum(byte b)
{
  out_message.add(b);
}

void MessageOut::write_byte(byte b)
{
  out_message.add(b);
  out_msg_chksum += int(b);
}

void MessageOut::write_uint(byte b)
{
  if (b > 127) {
    out_message.add(0xCC);
    out_msg_chksum += int(0xCC);  
  }
  out_message.add(b);
  out_msg_chksum += int(b);
}

void MessageOut::write_uint32(uint32_t w)
{
  int i;
  uint8_t b;
  uint32_t mask;

  mask = 0xFF000000;
  
  out_message.add(0xCE);
  out_msg_chksum += int(0xCE);  

  for (i = 3; i >= 0; i--) {
    b = (w & mask) >> (8*i);
    mask >>= 8;
    write_uint(b);
//    out_message.add(b);
//    out_msg_chksum += int(b);
  }
}


void MessageOut::write_prefixed_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len));
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void MessageOut::write_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(len + 0xa0));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}      
  
void MessageOut::write_long_string(const char *str)
{
  int len, i;

  len = strnlen(str, STR_LEN);
  write_byte(byte(0xd9));
  write_byte(byte(len));
  for (i=0; i<len; i++)
    write_byte(byte(str[i]));
}

void MessageOut::write_float (float flt)
{
  union {
    float val;
    byte b[4];
  } conv;
  int i;
   
  conv.val = flt;
  // Seems this creates the most significant byte in the last position, so for example
  // 120.0 = 0x42F00000 is stored as 0000F042  
   
  write_byte(0xca);
  for (i=3; i>=0; i--) {
    write_byte(byte(conv.b[i]));
  }
}

void MessageOut::write_onoff (bool onoff)
{
  byte b;

  if (onoff)
  // true is 'on'
    b = 0xc3;
  else
    b = 0xc2;
  write_byte(b);
}


void MessageOut::change_effect_parameter (char *pedal, int param, float val)
{
   if (cmd_base == 0x0100) 
     start_message (cmd_base + 0x04);
   else
     start_message (cmd_base + 0x37);
   write_prefixed_string (pedal);
   write_byte (byte(param));
   write_float(val);
   end_message();
}

void MessageOut::change_effect (char *pedal1, char *pedal2)
{
   start_message (cmd_base + 0x06);
   write_prefixed_string (pedal1);
   write_prefixed_string (pedal2);
   end_message();
}



void MessageOut::change_hardware_preset (uint8_t curr_preset, uint8_t preset_num)
{
   // preset_num is 0 to 3

   start_message (cmd_base + 0x38);
   write_byte (curr_preset);
   write_byte (preset_num)  ;     
   end_message();  
}


void MessageOut::turn_effect_onoff (char *pedal, bool onoff)
{
   start_message (cmd_base + 0x15);
   write_prefixed_string (pedal);
   write_onoff (onoff);
   end_message();
}

void MessageOut::get_serial()
{
   start_message (0x0223);
   end_message();  
}

void MessageOut::get_name()
{
   start_message (0x0211);
   end_message();  
}

void MessageOut::get_hardware_preset_number()
{
   start_message (0x0210);
   end_message();  
}

void MessageOut::get_checksum_info() {
   start_message (0x022a);
   write_byte(0x94);
   write_uint(0);
   write_uint(1);
   write_uint(2);
   write_uint(3);   
   end_message();   
}

void MessageOut::get_firmware() {
   start_message (0x022f);
   end_message(); 
}

void MessageOut::save_hardware_preset(uint8_t curr_preset, uint8_t preset_num)
{
   start_message (cmd_base + 0x27);
//   start_message (0x0327);
   write_byte (curr_preset);
   write_byte (preset_num);  
   end_message();
}

void MessageOut::send_firmware_version(uint32_t firmware)
{
   start_message (0x032f);
   write_uint32(firmware);  
   end_message();
}

void MessageOut::send_serial_number(char *serial)
{
   start_message (0x0323);
   write_prefixed_string(serial);
   end_message();
}

void MessageOut::send_ack(unsigned int cmdsub) {
   start_message (cmdsub);
   end_message();
}

void MessageOut::send_0x022a_info(byte v1, byte v2, byte v3, byte v4)
{
   start_message (0x032a);
   write_byte(0x94);
   write_uint(v1);
   write_uint(v2);
   write_uint(v3);
   write_uint(v4);      
   end_message();
}

void MessageOut::send_key_ack()
{
   start_message (0x0470);
   write_byte(0x00);
   end_message();
}

void MessageOut::send_preset_number(uint8_t preset_h, uint8_t preset_l)
{
   start_message (0x0310);
   write_byte(preset_h);
   write_byte(preset_l);
   end_message();
}

void MessageOut::tuner_on_off(bool onoff)
{
   start_message (0x0165);
   write_onoff (onoff);
   end_message();
}

void MessageOut::get_preset_details(unsigned int preset)
{
   int i;
   uint8_t h, l;

   uint_to_bytes(preset, &h, &l);
   
   start_message (0x0201);
   write_byte(h);
   write_byte(l);

   for (i=0; i<30; i++) {
     write_byte(0);
   }
   
   end_message(); 
}

void MessageOut::create_preset(SparkPreset *preset)
{
  int i, j, siz;

  start_message (cmd_base + 0x01);

  write_byte_no_chksum (0x00);
  write_byte_no_chksum (preset->preset_num);   
  write_long_string (preset->UUID);
  //write_string (preset->Name);
  if (strnlen (preset->Name, STR_LEN) > 31)
    write_long_string (preset->Name);
  else
    write_string (preset->Name);
    
  write_string (preset->Version);
  if (strnlen (preset->Description, STR_LEN) > 31)
    write_long_string (preset->Description);
  else
    write_string (preset->Description);
  write_string(preset->Icon);
  write_float (preset->BPM);
   
  write_byte (byte(0x90 + 7));       // always 7 pedals
  for (i=0; i<7; i++) {
    write_string (preset->effects[i].EffectName);
    write_onoff(preset->effects[i].OnOff);

    siz = preset->effects[i].NumParameters;
    write_byte ( 0x90 + siz); 
      
    for (j=0; j<siz; j++) {
      write_byte (j);
      write_byte (byte(0x91));
      write_float (preset->effects[i].Parameters[j]);
    }
  }
  write_byte_no_chksum (uint8_t(out_msg_chksum % 256));  
  end_message();
  
//  DEBUG("");
//  out_message.dump3();
}
