#ifndef SparkClass_h
#define SparkClass_h

#include <M5Stack.h>
#include <ArduinoJson.h>

#define BLK_SIZE 0xad
#define NUM_BLKS 5
#define DATA_SIZE 0x80

class SparkClass
{
   public:
      SparkClass();
      ~SparkClass();
    
      void start_message(int a_cmd, int a_sub_cmd, boolean a_multi);
      void end_message();

      void change_hardware_preset (int preset_num);
      void change_effect_parameter (char *pedal, int param, float val);
      void turn_effect_onoff (char *pedal, char *onoff);
      void change_effect (char *pedal1,  char *pedal2);
      void create_preset (const char *a_preset);
      void dump();   

      byte buf[NUM_BLKS][BLK_SIZE];
      int  last_pos;    // index of last byte written in buf
      int  last_block;  // index of last block being used
            
   private:
      void add_byte(byte b);
      void add_prefixed_string(const char *str);
      void add_long_string(const char *str);
      void add_string(const char *str);
      void add_float(float flt);
      void add_onoff(const char *onoff);
    
      int  data_pos;    // index of the raw data - ignores the 7bit expansion in the data block
    
      boolean multi;
      int cmd;
      int sub_cmd;

};

#endif
