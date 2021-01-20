#ifndef SparkClass_h
#define SparkClass_h

#include <M5Stack.h>
#include <ArduinoJson.h>

#define BLK_SIZE 0xad
#define NUM_BLKS 30
#define DATA_SIZE 0x80
#define IN_DATA_SIZE 28
#define MAX_MESSAGES 50

#define STR_LEN 40

typedef struct  {
   int start_filler;
   int preset_num;
   char UUID[STR_LEN];
   char Name[STR_LEN];
   char Version[STR_LEN];
   char Description[STR_LEN];
   char Icon[STR_LEN];
   float BPM;
   struct SparkEffects {
      char EffectName[STR_LEN];
      bool OnOff;
      int NumParameters;
      float Parameters[10];
   } effects[7];
   uint8_t end_filler;
} SparkPreset;

typedef struct {
  int cmd;
  int sub_cmd;
  int start_pos;
  int end_pos;
} MsgEntry;


class SparkClass
{
   public:
      SparkClass();
      ~SparkClass();
      
      //sending to Spark
      void start_message(int a_cmd, int a_sub_cmd, boolean a_multi);
      void end_message();

      void change_hardware_preset (int preset_num);
      void change_effect_parameter (char *pedal, int param, float val);
      void turn_effect_onoff (char *pedal, char *onoff);
      void change_effect (char *pedal1,  char *pedal2);
      void create_preset_json (const char *a_preset);
      void create_preset (SparkPreset& preset);
      
      // incoming from Spark      
      int get_data();
      void parse_data();

      void get_effect_parameter (int index, char *pedal, int *param, float *val);
      void get_effect_change(int index, char *pedal1, char *pedal2);
      void get_preset(int index, SparkPreset *preset);
      void start_reading(int start, bool a_multi);

      // general
      void dump();   
      void as_hex();

      // public variables
      byte buf[NUM_BLKS][BLK_SIZE];

      int last_pos;    // index of last byte written in buf
      int last_block;  // index of last block being used

      MsgEntry messages[MAX_MESSAGES];
      int num_messages;

            
   private:
      void add_byte(byte b);
      void add_prefixed_string(const char *str);
      void add_long_string(const char *str);
      void add_string(const char *str);
      void add_float(float flt);
      void add_onoff(const char *onoff);

      bool read_string(char *str);
      bool read_prefixed_string(char *str);
      bool read_onoff();
      float read_float();
      byte read_byte();
      
      void get_block(int block);

      int seq;
      
      int data_pos;    // logical index of the actual data - ignores the 7bit expansion in the data block
      int chunk_offset;
      boolean multi;
      int cmd;
      int sub_cmd;
      
};

#endif
