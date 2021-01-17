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

// ----------------------------------------------------
//  Routines to write to the Spark
//

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

// ----------------------------------------------------
//  Routines to read from the Spark
//



// Functions to read a whole block from the Spark
// 
// Store in the SparkClass buffer to save data copying

int read_bt() {
  while (!SerialBT.available()) delay(10);
  return SerialBT.read();
}

void SparkClass::get_block(int block)
{
   int r1, r2;
   bool started;
   int i, len; 

   // look for the 0x01fe start signature
   
   started = false;
   r1 = read_bt();
   while (!started) {
      r2 = read_bt();
      if (r1==0x01 && r2 == 0xfe)
         started = true;
      else
         r1 = r2;
   };

   buf[block][0]=r1;
   buf[block][1]=r2;
   // we have found the start signature so read up until the length byte
   for (i=2; i<7; i++)
      buf[block][i] = read_bt();

   len = buf[block][6];
   if (len > BLK_SIZE) {
      Serial.print(len, HEX);
      Serial.println(" is too big for a block, so halting");
      while (true);
   };
      
   for (i=7; i<len; i++)
      buf[block][i] = read_bt();

  last_block = 0;
  last_pos = i-1;
}
  

void SparkClass::get_data()
{

   int block;
   bool is_last_block;

   int blk_len; 
   int num_chunks, this_chunk;
   int seq, cmd, sub_cmd;
   int directn;

   int pos;

   block = 0;
   is_last_block = false;

   while (!is_last_block) {
      if (block >= NUM_BLKS) {
         Serial.print(block);
         Serial.println(" BLOCKS SO FAR - THIS DATA IS TOO BIG FOR MY NUM_BLKS");
         break;
      }
      get_block(block);
      blk_len = buf[block][6];
      directn = buf[block][4]; // use the 0x53 or the 0x41 and ignore the second byte
      seq     = buf[block][18];
      cmd     = buf[block][20];
      sub_cmd = buf[block][21];

 
      if (directn == 0x53 && cmd == 0x01 && sub_cmd != 0x04)
          // the app sent a message that needs a response
          send_ack(seq, cmd);
            
      //now we need to see if this is the last block

      // if the block length is less than the max size then
      // definitely last block
      // could be a full block and still last one
      // but to be full surely means it is a multi-block as
      // other messages are always small
      // so need to check the chunk counts - in different places
      // depending on whether    

      if  (directn == 0x53) 
         if (blk_len < 0xad)
            is_last_block = true;
         else {
            // this is sent to Spark so will have a chunk header at top
            num_chunks = buf[block][23];
            this_chunk = buf[block][24];
            if (this_chunk + 1 == num_chunks)
               is_last_block = true;
         }
      if (directn == 0x41) 
         if (blk_len < 0x6a)
            is_last_block = true;
         else {
            // this is from Spark so chunk header could be anywhere
            // so search from the end
            // there must be one within the block and it is outside the 8bit byte so the search is ok

            for (pos = 0x6a-2; pos >= 22 && !(buf[block][pos] == 0xf0 && buf[block][pos+1] == 0x01); pos--);
            num_chunks = buf[block][pos+7];
            this_chunk = buf[block][pos+8];
            if (this_chunk + 1 == num_chunks)
               is_last_block = true;
         }
      last_block = block;
      block++;
   }
}

#define BLK(x) (int((x)/90))
#define BLK_POS(x) (16 + (x)%90)

void SparkClass::parse_data() {
   int pos, start_pos, end_pos, blk_pos, num, chunk_len;
   int cmd, sub_cmd;
   bool finished;
   char a_str[50];

   num_messages=0;
      
   start_pos = 0;
   finished = false;

   while (!finished){
      pos = start_pos;
      cmd = buf[BLK(pos+4)][BLK_POS(pos+4)];
      sub_cmd = buf[BLK(pos+5)][BLK_POS(pos+5)];

   
      if (cmd == 0x03 && sub_cmd == 0x01) {
         // multi-block has internal format
         num = buf[BLK(pos+7)][BLK_POS(pos+7)];
      
         pos += 39 * (num-1);
//         blk = BLK(pos);
//         blk_size = buf[blk][6];
      
         chunk_len = buf[BLK(pos+6+3)][BLK_POS(pos+6+3)];
         end_pos = pos + 10 + int ((chunk_len+2)/7) + chunk_len;
      }
      else {
       // search for the 0xf7
         pos = start_pos+6;

         while (buf[BLK(pos)][BLK_POS(pos)] != 0xf7) pos++;
         end_pos = pos;
      }
      messages[num_messages].cmd = cmd;
      messages[num_messages].sub_cmd = sub_cmd;
      messages[num_messages].start_pos = start_pos;
      messages[num_messages].end_pos = end_pos;             
      num_messages++;
      
      if (buf[BLK(end_pos)][6] -1  == BLK_POS(end_pos)) finished = true;
      start_pos = end_pos+1;
   }
}

// Routines to interpret the data

void SparkClass::start_reading(int start, bool a_multi)
{
   scr.chunk_offset = start;
   data_pos = 0;
   multi = a_multi;
}

byte SparkClass::read_byte()
{

   int blk, multi_bytes, temp_pos, slice_pos, byte_pos, bit_pos, pos, chunk, chunk_data_size;
   int bits, dat;
   
   chunk_data_size = multi ? 25: 1000; // kludge until I refactor all of this
   chunk = int(data_pos / chunk_data_size);
   multi_bytes = multi ? 3 : 0;
      
   // work out where to place the data byte and where the 8th bit should go
   temp_pos = (data_pos % chunk_data_size) + multi_bytes;
   slice_pos = 8 * int (temp_pos / 7);
   byte_pos = temp_pos % 7;
   bit_pos = scr.chunk_offset + chunk*39 + slice_pos + 6;
   pos = scr.chunk_offset + chunk*39 + byte_pos + slice_pos + 7;
      
   
   bits = buf[BLK(bit_pos)][BLK_POS(bit_pos)];
   dat = buf[BLK(pos)][BLK_POS(pos)];
   
   if (bits &  (1<<byte_pos))
      dat |= 0x80;

/*   
   Serial.print(pos);
   Serial.print(" ");
   Serial.print(bit_pos);
   Serial.print(" ");
   Serial.print(dat, HEX);
   Serial.print(" ");
   Serial.println(bits, BIN);  
*/   
   data_pos++;
   return dat;
}   
   

bool SparkClass::read_string(char *str)
{
   int a, i, len;

   a=read_byte();
   if (a == 0xd9) 
      len = read_byte();
   else if (a > 0xa0)
      len = a - 0xa0;
   else {
      a=read_byte();
      len = a - 0xa0;
   }

   if (len < STR_LEN) {
      for (i=0; i<len; i++) {
         a = read_byte();
         str[i]=a;
      }
      str[i] = '\0';
      return true;
   }
   else
      return -1;
}

bool SparkClass::read_prefixed_string(char *str)
{
   int a, i, len;

   a=read_byte(); 
   a=read_byte();
   
   len = a-0xa0;
   

   if (len < STR_LEN) {
      for (i=0; i<len; i++) {
         a = read_byte();
         str[i]=a;
      }
      str[i] = '\0';
      return true;
   }
   else
      return -1;
}

float SparkClass::read_float()
{
   union {
      float val;
      byte b[4];
   } conv;   
   int a, i;

   a = read_byte();  // should be 0xca
  
   // Seems this creates the most significant byte in the last position, so for example
   // 120.0 = 0x42F00000 is stored as 0000F042  
   
   for (i=3; i>=0; i--) {
      a = read_byte();
      conv.b[i] = a;
   } 
   return conv.val;
}
 


bool SparkClass::read_onoff()
{
   int a;
   
   a = read_byte();
   if (a == 0xc3)
      return true;
   else // 0xc2
      return false;
}

// The functions to get the messages


void SparkClass::get_effect_parameter (int index, char *pedal, int *param, float *val)
{
   int i, start_p, end_p;
   
   start_p = messages[index].start_pos;
   end_p = messages[index].end_pos;

   start_reading(start_p, false);

   read_string(pedal);
   *param = read_byte();
   *val = read_float();
}

void SparkClass::get_effect_change(int index, char *pedal1, char *pedal2)
{
   int i, start_p, end_p;
   
   start_p = messages[index].start_pos;
   end_p = messages[index].end_pos;

   start_reading(start_p, false);

   read_string(pedal1);
   read_string(pedal2);
}

void SparkClass::get_preset(int index, SparkPreset *preset)
{
   int i, j, start_p, end_p;
   char a_str[STR_LEN];
   float flt;
   bool onoff;
   int t;
   
   start_p = messages[index].start_pos;
   end_p = messages[index].end_pos;

   start_reading(start_p, true);   // the only multi-chunk message

   read_byte();
   preset->preset_num = read_byte();
   read_string(preset->UUID); 
   read_string(preset->Name);
   read_string(preset->Version);
   read_string(preset->Description);
   read_string(preset->Icon);
   preset->BPM = read_float();

   for (j=0; j<7; j++) {
      read_string(preset->effects[j].EffectName);
      preset->effects[j].OnOff = read_onoff();   
      preset->effects[j].NumParameters = read_byte()- 0x90;
      for (i=0; i<preset->effects[j].NumParameters; i++) {
         read_byte();
         read_byte();
         preset->effects[j].Parameters[i] = read_float();
      }
   }
}
