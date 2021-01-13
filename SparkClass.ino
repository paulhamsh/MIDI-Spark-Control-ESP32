#include <M5Stack.h>
#include <ArduinoJson.h>
#include "SparkClass.h"

SparkClass::SparkClass()
{ 
   last_pos = 0;
   last_block = 0;

   data_pos = -1;
   multi = false;
}

SparkClass::~SparkClass()
{
   last_pos = -1;
}


void SparkClass::as_hex()
{
   int b, p, num;

   for (b = 0; b <= last_block; b++) {
      Serial.println();
      // num = (b == last_block) ? last_pos : BLK_SIZE - 1;
      num = buf[b][6]-1;
      for (p = 0; p <= num; p++) {
         if (buf[b][p] < 16) Serial.print("0"); 
         Serial.print(buf[b][p], HEX);
      }
   }
   Serial.println();
}


void SparkClass::dump()
{
   int b, p, num;

   for (b = 0; b <= last_block; b++) {
      Serial.println();
      Serial.println("------------------");
      Serial.print("Block ");
      Serial.println(b);
      // num = (b == last_block) ? last_pos : BLK_SIZE - 1;
      num = buf[b][6]-1;
      for (p = 0; p <= num; p++) {
         if (buf[b][p] < 16) Serial.print("0"); 
         Serial.print(buf[b][p], HEX);
         Serial.print(" ");
         if (p < 21 && p%8==7) Serial.println();
         if (p == 21) Serial.println();
         if (p > 21 && (p-22) % 8 == 7) Serial.println();
      }
   }
   Serial.println();
}

// Private functions

void SparkClass::add_byte(byte b)
{
   int blk, pos, temp_pos, multi_bytes, slice_pos, byte_pos, bit_pos;

   if (data_pos <= DATA_SIZE * NUM_BLKS) {
      blk = int(data_pos / 0x80);
      multi_bytes = multi ? 3 : 0;
      
      // work out where to place the data byte and where the 8th bit should go
      temp_pos = (data_pos % 0x80) + multi_bytes;
      slice_pos = 8 * int (temp_pos / 7);
      byte_pos = temp_pos % 7;
      bit_pos = 22 + slice_pos;
      pos = 23 +  byte_pos + slice_pos;
      
      // store the low 7 bits
      buf[blk][pos] = b & 0x7f;
      
      // update 8th bit if needed
      if (b & 0x80)
         buf[blk][bit_pos] |= (1<<byte_pos);
      
      data_pos++;
      last_block = blk;
      last_pos = pos;
   }
}

void SparkClass::add_prefixed_string(const char *str)
{
   int len, i;

   len = strlen(str);
   add_byte(byte(len));
   add_byte(byte(len + 0xa0));
   for (i=0; i<len; i++)
      add_byte(byte(str[i]));
}

void SparkClass::add_string(const char *str)
{
   int len, i;

   len = strlen(str);
   add_byte(byte(len + 0xa0));
   for (i=0; i<len; i++)
      add_byte(byte(str[i]));
}      
  
void SparkClass::add_long_string(const char *str)
{
   int len, i;

   len = strlen(str);
   add_byte(byte(0xd9));
   add_byte(byte(len));
   for (i=0; i<len; i++)
      add_byte(byte(str[i]));
}

void SparkClass::add_float (float flt)
{
   union {
      float val;
      byte b[4];
   } conv;
   int i;
   
   conv.val = flt;
   // Seems this creates the most significant byte in the last position, so for example
   // 120.0 = 0x42F00000 is stored as 0000F042  
   
   add_byte(0xca);
   for (i=3; i>=0; i--) {
      add_byte(byte(conv.b[i]));
   }
}

void SparkClass::add_onoff (const char *onoff)
{
   byte b;

   if (strcmp(onoff, "On")==0)
      b = 0xc3;
   else
      b = 0xc2;
   add_byte(b);
}

// Public functions

void SparkClass::start_message(int a_cmd, int a_sub_cmd, boolean a_multi)
{
   cmd = a_cmd;
   sub_cmd = a_sub_cmd;
   multi = a_multi;
   
   last_pos = 0;
   last_block = 0;
   data_pos = 0;

   memset(buf, 0, sizeof(buf));
}

void SparkClass::end_message()
{
   int num_chunks, data_len, last_chunk_data_len;
   int pos;
   int i;

   // data_pos is pointing to next empty space, but starts at zero so is actually just the length
   data_len = data_pos; 
   num_chunks = last_block + 1;

   last_chunk_data_len = data_len % 0x80;

   // for multi_chunk messages we need to update the chunk data header 3 bytes
   if (multi)
      for (i = 0; i < num_chunks; i++) {
         buf[i][23] = byte(num_chunks);
         buf[i][24] = byte(i);
         if (i < num_chunks-1) {
            buf[i][25] = 0x00;  // 0x80 without the 8th bit
            buf[i][22] |= 0x04; // 8th bit in known position
            }
         else
            buf[i][25] = last_chunk_data_len;
         };

   for (i=0; i < num_chunks; i++)
   {
       // place the final 0xf7
       if (i == num_chunks-1)
          pos =  ++last_pos;
       else
          pos = 172; 
       buf[i][pos] = 0xf7;

       // build header
       buf[i][0] = 0x01;
       buf[i][1] = 0xfe;
       buf[i][4] = 0x53;
       buf[i][5] = 0xfe;
       
       // add block size
       buf[i][6] = pos+1;

       // add chunk header
       buf[i][16] = 0xf0;
       buf[i][17] = 0x01;
       buf[i][18] = 0x3a;  // sequence number - this is a random one
       buf[i][19] = 0x15;  // unknown what this is
       buf[i][20] = cmd;       
       buf[i][21] = sub_cmd;
   }
}


void SparkClass::change_effect_parameter (char *pedal, int param, float val)
{
   int cmd, sub_cmd;
   cmd = 0x01;
   sub_cmd = 0x04;
    
   start_message (cmd, sub_cmd, false);
   add_prefixed_string (pedal);
   add_byte (byte(param));
   add_float(val);
   end_message();
}


void SparkClass::change_effect (char *pedal1, char *pedal2)
{
   int cmd, sub_cmd;
   cmd = 0x01;
   sub_cmd = 0x06;

   start_message (cmd, sub_cmd, false);
   add_prefixed_string (pedal1);
   add_prefixed_string (pedal2);
   end_message();
}

void SparkClass::change_hardware_preset (int preset_num)
{
   // preset_num is 0 to 3
   int cmd, sub_cmd;
    
   cmd = 0x01;
   sub_cmd = 0x38;

   start_message (cmd, sub_cmd, false);
   add_byte (byte(0));
   add_byte (byte(preset_num))  ;     
   end_message();  
}

void SparkClass::turn_effect_onoff (char *pedal, char *onoff)
{
   int cmd;
   
   cmd = 0x01;
   sub_cmd = 0x15;

   start_message (cmd, sub_cmd, false);
   add_prefixed_string (pedal);
   add_onoff (onoff);
   end_message();
}



void SparkClass::create_preset_json (const char *a_preset)
{
   int cmd, sub_cmd;
   int i, siz;
  
   const char *Description;
   const char *UUID;
   const char *Name;
   const char *Version;
   const char *Icon;
   float BPM;
   float Param;
   const char *OnOff;
   const char *PedalName;
   int EndFiller;

   cmd =     0x01;
   sub_cmd = 0x01;

   DynamicJsonDocument preset(4000);
   deserializeJson(preset, a_preset);

   start_message (cmd, sub_cmd, true);
 
   UUID = preset["UUID"];
   Name = preset["Name"];
   Version = preset["Version"];
   Description = preset["Description"];      
   Icon = preset["Icon"];
   BPM = preset["BPM"];
   EndFiller = preset["EndFiller"];
   
   add_byte (0x00);
   add_byte (0x7f);   
   add_long_string (UUID);
   add_string (Name);
   add_string (Version);
   if (strlen (Description) > 31)
      add_long_string (Description);
   else
      add_string (Description);
   add_string(Icon);
   add_float (BPM);

   
   add_byte (byte(0x90 + 7));       // always 7 pedals

   JsonArray Pedals = preset["Pedals"]; //[0]["Parameters"];
   for (JsonVariant pedal : Pedals) {
      PedalName = pedal["Name"];
      OnOff = pedal["OnOff"];
      
      add_string (PedalName);
      add_onoff (OnOff);
      
      i = 0;

      JsonArray Parameters = pedal["Parameters"];
      siz = Parameters.size();
      add_byte ( 0x90 + siz); 
      
      for (JsonVariant params: Parameters) {
         Param = params;
         add_byte (i++);
         add_byte (byte(0x91));
         add_float (Param);
      }
   }
   add_byte (EndFiller);  
   end_message();
}

void SparkClass::create_preset (SparkPreset &preset)
{
   int cmd, sub_cmd;
   int i, j, siz;
  
   const char *Description;
   const char *UUID;
   const char *Name;
   const char *Version;
   const char *Icon;
   float BPM;
   float Param;
   const char *OnOff;
   const char *PedalName;
   int EndFiller;

   cmd =     0x01;
   sub_cmd = 0x01;
   start_message (cmd, sub_cmd, true);

   add_byte (0x00);
   add_byte (preset.preset_num);   
   add_long_string (preset.UUID);
   add_string (preset.Name);
   add_string (preset.Version);
   if (strlen (preset.Description) > 31)
      add_long_string (preset.Description);
   else
      add_string (preset.Description);
   add_string(preset.Icon);
   add_float (preset.BPM);

   
   add_byte (byte(0x90 + 7));       // always 7 pedals

   for (i=0; i<7; i++) {
      
      add_string (preset.effects[i].EffectName);
      if (preset.effects[i].OnOff)
         add_onoff("On");
      else 
         add_onoff ("Off");

      siz = preset.effects[i].NumParameters;
      add_byte ( 0x90 + siz); 
      
      for (j=0; j<siz; j++) {
         add_byte (j);
         add_byte (byte(0x91));
         add_float (preset.effects[i].Parameters[j]);
      }
   }
   add_byte (preset.end_filler);  
   end_message();
}