#ifndef SparkIO_h
#define SparkIO_h

#include "RingBuffer.h"
#include "SparkStructures.h"
#include "SparkComms.h"

#define MAX_IO_BUFFER 2048

uint8_t license_key[64];
bool is_spark_mini = false;

// MESSAGE INPUT CLASS
class MessageIn
{
  public:
    MessageIn() {};
    bool get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);
    RingBuffer *in_message;

    void read_string(char *str);
    void read_prefixed_string(char *str);
    void read_onoff(bool *b);
    void read_float(float *f);
    void read_uint(uint8_t *b);
    void read_general_uint(uint32_t *b);
    void read_byte(uint8_t *b);
};

class SparkMessageIn: public MessageIn
{
  public:
    SparkMessageIn() {};
    void set(RingBuffer *messages);
};

class AppMessageIn: public MessageIn
{
  public:
    AppMessageIn() {};
    void set(RingBuffer *messages);
};

// MESSAGE OUTPUT CLASS
class MessageOut
{
  public:
    MessageOut() {};
    
    // creating messages to send
    void start_message(int cmdsub);
    void end_message();
    void write_byte(byte b);
    void write_byte_no_chksum(byte b);
    
    void write_uint(byte b);
    void write_prefixed_string(const char *str);
    void write_long_string(const char *str);
    void write_string(const char *str);
    void write_float(float flt);
    void write_onoff(bool onoff);
    void write_uint32(uint32_t w);

    void create_preset(SparkPreset *preset);
    void turn_effect_onoff(char *pedal, bool onoff);
    void change_hardware_preset(uint8_t curr_preset, uint8_t preset_num);
    void change_effect(char *pedal1, char *pedal2);
    void change_effect_parameter(char *pedal, int param, float val);
    void get_serial();
    void get_name();
    void get_hardware_preset_number();
    void get_preset_details(unsigned int preset);
    void get_checksum_info();
    void get_firmware();
    void save_hardware_preset(uint8_t curr_preset, uint8_t preset_num);
    void send_firmware_version(uint32_t firmware);
    void send_0x022a_info(byte v1, byte v2, byte v3, byte v4);  
    void send_preset_number(uint8_t preset_h, uint8_t preset_l);
    void send_key_ack();
    void send_serial_number(char *serial);
    void send_ack(unsigned int cmdsub);
    // trial message
    void tuner_on_off(bool onoff);

    RingBuffer *out_message;
    int cmd_base;
    int out_msg_chksum;
};

class SparkMessageOut: public MessageOut
{
  public:
    SparkMessageOut() {};
    void set(RingBuffer *messages);
};

class AppMessageOut: public MessageOut
{
  public:
    AppMessageOut() {};
    void set(RingBuffer *messages);
};

/////////////


    void spark_start(bool passthru);

    void spark_process();

    // processing received messages    
//    bool spark_get_message(unsigned int *cmdsub, SparkMessage *msg, SparkPreset *preset);

    // sending data

    RingBuffer sp_in_message;
    SparkMessageIn spark_msg_in;

    SparkMessageOut spark_msg_out;
    RingBuffer sp_out_message;
    
    bool sp_ok_to_send;          
    bool app_ok_to_send;

    uint8_t sp_rec_seq;
    uint8_t app_rec_seq;
    
    void app_process();

    RingBuffer app_in_message;
    AppMessageIn app_msg_in;

    AppMessageOut app_msg_out;
    RingBuffer app_out_message;
    #endif
      
