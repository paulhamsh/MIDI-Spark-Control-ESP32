#include <M5Stack.h>
#include <ArduinoJson.h>
#include "SparkClass.h"
#include "SparkPresets.h"
#include "MIDIMap.h"
#include <usbh_midi.h>
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32

// Bluetooth vars
#define SPARK_NAME "Spark 40 Audio"
#define MY_NAME    "MIDI Spark"

BluetoothSerial SerialBT;

// MIDI vars
USB Usb;
USBH_MIDI MIDIUSBH(&Usb);


// Spark vars
char my_preset[]="{\"PresetNumber\": [0, 127], "
              "\"UUID\": \"07079063-94A9-41B1-AB1D-02CBC5D00790\", "
              "\"Name\": \"Silver Ship\", "
              "\"Version\": \"0.7\", "
              "\"Description\": \"1-Clean\", "
              "\"Icon\": \"icon.png\", "
              "\"BPM\": 120.0, "
              "\"Pedals\": ["
                  "{\"Name\": \"bias.noisegate\", "
                   "\"OnOff\": \"Off\", "
                   "\"Parameters\": [0.138313, 0.224643, 0.0]}, "
                  "{\"Name\": \"LA2AComp\", "
                   "\"OnOff\": \"On\", "
                   "\"Parameters\": [0.0, 0.852394, 0.373072]}, "
                  "{\"Name\": \"Booster\", "
                   "\"OnOff\": \"Off\", "
                   "\"Parameters\": [0.722592]}, "
                  "{\"Name\": \"RolandJC120\", "
                   "\"OnOff\": \"On\", "
                   "\"Parameters\": [0.632231, 0.28182, 0.158359, 0.67132, 0.805785]}, "
                  "{\"Name\": \"Cloner\", "
                   "\"OnOff\": \"On\", "
                   "\"Parameters\": [0.199593, 0.0]}, "
                  "{\"Name\": \"VintageDelay\", "
                   "\"OnOff\": \"Off\", "
                   "\"Parameters\": [0.378739, 0.425745, 0.419816, 1.0]}, "
                  "{\"Name\": \"bias.reverb\", "
                   "\"OnOff\": \"On\", "
                   "\"Parameters\": [0.285714, 0.408354, 0.289489, 0.388317, 0.582143, 0.65, 0.2]}], "
              "\"EndFiller\": 180}";    // no space in key name allowed

SparkClass sc1, sc2, sc3, sc4, scr;
SparkClass sc_setpreset7f;



// ------------------------------------------------------------------------------------------
// Bluetooth routines
  
void connect_to_spark() {
   bool connected = false;
   int rec;
   
   if (!SerialBT.begin (MY_NAME, true)) {
      M5.Lcd.println("Bluetooth initialisation failure");
      while (true);
   }
   
   M5.Lcd.println("Connecting to Spark");
   while (!connected) {
      connected = SerialBT.connect(SPARK_NAME);
      if (connected && SerialBT.hasClient()) {
         M5.Lcd.println("Connected to Spark");
      }
      else {
         connected = false;
         delay(3000);
      }
   }
   // flush anything read from Spark - just in case
   while (SerialBT.available())
      rec = SerialBT.read(); 
}

// Send messages to the Spark (and receive an acknowledgement where appropriate

void send_bt(SparkClass& spark_class)
{
   int i;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
//      delay(200);   
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1);
//   delay(200);
}


// Helper functions to send an acknoweledgement and to send a requesjames luit for a preset

void send_ack(int seq, int cmd)
{
   byte ack[]{  0x01, 0xfe, 0x00, 0x00, 0x41, 0xff, 0x17, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                0xf0, 0x01, 0xff, 0x00, 0x04, 0xff, 0xf7};
   ack[18] = seq;
   ack[21] = cmd;            

   SerialBT.write(ack, sizeof(ack));
}

void send_preset_request(int preset)
{
   byte req[]{0x01, 0xfe, 0x00, 0x00, 0x53, 0xfe, 0x3c, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0xf0, 0x01, 0x04, 0x00, 0x02, 0x01,
              0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00, 0x00, 0xf7};
 
   req[24] = preset;
   SerialBT.write(req, sizeof(req));      
}

// Functions to read a whole block from the Spark
// 
// Store in the SparkClass buffer to save data copying


void get_block(SparkClass& sc, int block)
{
   int r1, r2;
   bool started;
   int i, len; 

   // look for the 0x01fe start signature
   
   started = false;
   r1 = SerialBT.read();
   while (!started) {
      r2 = SerialBT.read();
      if (r1==0x01 && r2 == 0xfe)
         started = true;
      else
         r1 = r2;
   };

   sc.buf[block][0]=r1;
   sc.buf[block][1]=r2;
   // we have found the start signature so read up until the length byte
   for (i=2; i<7; i++)
      sc.buf[block][i] = SerialBT.read();

   len = sc.buf[block][6];
   if (len > BLK_SIZE) {
      Serial.print(len, HEX);
      Serial.println(" is too big for a block, so halting");
      while (true);
   };
      
   for (i=7; i<len; i++)
      sc.buf[block][i] = SerialBT.read();

  sc.last_block = 0;
  sc.last_pos = i-1;
}
  

void get_data(SparkClass& sc)
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
      get_block(sc, block);
      blk_len = sc.buf[block][6];
      directn = sc.buf[block][4]; // use the 0x53 or the 0x41 and ignore the second byte
      seq     = sc.buf[block][18];
      cmd     = sc.buf[block][20];
      sub_cmd = sc.buf[block][21];

 
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
            num_chunks = sc.buf[block][23];
            this_chunk = sc.buf[block][24];
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

            for (pos = 0x6a-2; pos >= 22 && !(sc.buf[block][pos] == 0xf0 && sc.buf[block][pos+1] == 0x01); pos--);
            num_chunks = sc.buf[block][pos+7];
            this_chunk = sc.buf[block][pos+8];
            if (this_chunk + 1 == num_chunks)
               is_last_block = true;
         }
      sc.last_block = block;
      block++;
      }
}

/* No longer needed as async receive
 *  
 *  */

void send_receive_bt2(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      // read the ACK
      get_data(scr);
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 
   // read the ACK
   get_data(scr);
}

void send_receive_bt(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      // read the ACK
      delay(100);
      while (SerialBT.available()) SerialBT.read(); 
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 
   
   // read the ACK
   delay(100);
   while (SerialBT.available()) SerialBT.read(); 
}

// ------------------------------------------------------------------------------------------
// MIDI routines
  
void start_midi() {
   if (Usb.Init() == -1) {
      M5.Lcd.println("USB host init failed");
      while (true); //halt
   }
}

int get_current_index(char *type) {
   int i;

   if      (!strcmp(type, "Gate"))   i = 0;
   else if (!strcmp(type, "Comp"))   i = 1;
   else if (!strcmp(type, "Drive"))  i = 2;           
   else if (!strcmp(type, "Amp"))    i = 3;
   else if (!strcmp(type, "Mod"))    i = 4;
   else if (!strcmp(type, "Delay"))  i = 5;
   else if (!strcmp(type, "Reverb")) i = 6;
   else i = -1;
   return i;
}
void midi_event() {
   uint8_t receive_buf[MIDI_EVENT_PACKET_SIZE];
   uint16_t received;

   uint8_t *p;
   
   int return_code = 0;     //return code
   int count = 0;

   int num_noteon_map, num_cc_map, num_presets;
   bool found_midi_map;
   bool found_preset;
   
   int header, chan, miditype, dat1, dat2;
   int i, j, k;
   int curr_ind;

   return_code = MIDIUSBH.RecvData(&received, receive_buf);

   //check there is data
   if (return_code != 0 || received == 0) return;
   if (receive_buf[0] == 0 && receive_buf[1] == 0 && receive_buf[2] == 0 && receive_buf[3] == 0) return;
   
   while (count < received)  {
      p = &receive_buf[count];
      if (p[0] == 0 && p[1] == 0) break; //data end

      header =   p[0] & 0x0f;
      chan =     p[1] & 0x0f + 1;
      miditype = p[1] & 0xf0;
      dat1 =     p[2];
      dat2 =     p[3];

      count += 4;
          
      M5.Lcd.print(miditype, HEX);
      M5.Lcd.print(" ");
      M5.Lcd.print(dat1, HEX);
      M5.Lcd.print(" ");
      M5.Lcd.print(dat2, HEX);
      M5.Lcd.println();

      /*
      * 
      * hardware preset:
      *    send new preset
      *    get preset data from Spark
      *    read the message
      *    update the current[] for each new effect
      *    
      * change parameter:
      *    use parameter type (amp, chorus...)
      *    look up current[] effect
      *    send change effect parameter
      *    
      * change effect:
      *    use parameter type (amp...)
      *    look up current[] effect
      *    send change effect (old, new)
      *    change current effect to new
      *    
      * set effect on off:
      *    use parameter type (amp....)
      *    look up current[] effect
      *    send effect onoff
      *    
      * new preset:
      *    package up preset message
      *    send preset
      *    send change to hardware 0x7f
      *    update the current[] for each new effect
      *    
      */

      num_noteon_map = sizeof(NoteOn) / sizeof(MIDIMap);
      num_cc_map = sizeof(CC) / sizeof(MIDIMap);
      num_presets = sizeof(presets) / sizeof(SparkPreset *);
//      num_presets = 24;
      
      // Handle Note On
      found_midi_map = false;
      if (miditype == 0x90) {
         for (i = 0; i < num_noteon_map; i++)
            if (NoteOn[i].note == dat1) {
               found_midi_map = true;
               break;
            }

         if (found_midi_map) {   
            if (!strcmp(NoteOn[i].command, "HwPreset")) {
               sc2.change_hardware_preset(NoteOn[i].value);
               send_receive_bt(sc2);
               
               Serial.print("Change hardware preset  ");
               Serial.println(NoteOn[i].value);

               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
               // need to get the preset details now to update our local info
                              
            }
            else if (!strcmp(NoteOn[i].command, "EfectOnOff")) {
              // find the current[] effect of this type
              curr_ind = get_current_index(NoteOn[i].param1);
              if (curr_ind >= 0) {
                 sc2.turn_effect_onoff(current_effects[curr_ind], NoteOn[i].param2);
                 send_receive_bt(sc2);
                 
                 Serial.print("Change effect on/off ");
                 Serial.print(current_effects[curr_ind]);
                 Serial.print (" ");
                 Serial.println (NoteOn[i].param2);
              } 
            }
            else if (!strcmp(NoteOn[i].command, "ChangePreset")) {
               // find the current[] effect of this type
               found_preset = false;

               for (j = 0; j < num_presets; j++) {
                  if (!strcmp(presets[j]->Name, NoteOn[i].param1)) {
                     found_preset = true;
                     break;
                  }
               }
               if (found_preset) {
                  sc2.create_preset(*presets[j]);
                  sc2.dump();
                  send_receive_bt(sc2);
                  // and send the select hardware 7f message too
                  send_receive_bt(sc_setpreset7f);
                 
                  Serial.print("Change preset ");
                  Serial.println(presets[j]->Name);
                  for (k = 0; k < 7; k++) Serial.println(current_effects[k]);
                  for (k = 0; k < 7; k++) strcpy(current_effects[k], presets[j]->effects[k].EffectName);
                  for (k = 0; k < 7; k++) Serial.println(current_effects[k]);

                  // need to get the preset details now to update our local info
                  // need to get the preset details now to update our local info
                  // need to get the preset details now to update our local info
                  // need to get the preset details now to update our local info                  
                  // need to get the preset details now to update our local info

               }
            }
            else if (!strcmp(NoteOn[i].command, "ChangeEffect")) {
               // find the current[] effect of this type
               curr_ind = get_current_index(NoteOn[i].param1);
               if (curr_ind >= 0) {
                  sc2.change_effect(current_effects[curr_ind], NoteOn[i].param2);
                  send_receive_bt(sc2);
                 
                  Serial.print("Change effect ");
                  Serial.print(current_effects[curr_ind]);
                  Serial.print(" ");
                  Serial.println(NoteOn[i].param2);
                  // update our current[] map
                  strcpy(current_effects[curr_ind], NoteOn[i].param2);
               } 
            }
         }
      }
      // Handle CC
      else if (miditype == 0xB0) {
         for (i = 0; i < num_cc_map; i++)
            if (CC[i].note == dat1) {
               found_midi_map = true;
               break;
            }
         if (found_midi_map) {
            if (!strcmp(CC[i].command, "ChangeParam")) {
               curr_ind = get_current_index(CC[i].param1);
               if (curr_ind >= 0) {
                  sc2.change_effect_parameter(current_effects[curr_ind], CC[i].value, dat2);
                  send_bt(sc2);

                  Serial.print("Change param  ");
                  Serial.print(CC[i].param1);
                  Serial.print(" ");
                  Serial.print(current_effects[curr_ind]);
                  Serial.print(" ");
                  Serial.print(CC[i].value);
                  Serial.print(" ");
                  Serial.println(dat2);
               }
            }  
         }                   
      }
   }
}



// ------------------------------------------------------------------------------------------

void setup() {
   M5.begin();
   M5.Power.begin();
   M5.Lcd.fillScreen(TFT_BLACK);
   M5.Lcd.setTextSize(2);
   M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
   M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLACK);
   M5.Lcd.println("Spark Class Core");
  
   Serial.begin(115200);
   Serial.println("Spark Class Core");

   start_midi();
   connect_to_spark();

   // set up the change to 7f message for when we send a full preset
   sc_setpreset7f.change_hardware_preset(0x7f);
   
}

void loop() {
   M5.update();
   Usb.Task();
   
   if (M5.BtnA.wasReleased()) {
      M5.Lcd.println("Hardware preset 0");
      sc2.change_hardware_preset(0);
      send_bt(sc2);
  } 
   if (M5.BtnB.wasReleased()) {

      M5.Lcd.println("Spooky Melody");

byte SpookyMelodyA[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x0c,0x16,0x01,0x01,0x24,0x04,0x00,0x00,0x00,0x7f,0x59,0x24,0x00,0x39,0x34,0x31,0x30,0x39,0x34,0x31,0x00,0x38,0x2d,0x45,0x37,0x44,0x39,0x2d,0x00,0x34,0x42,0x39,0x39,0x2d,0x38,0x33,0x00,0x46,0x37,0x2d,0x44,0x44,0x42,0x31,0x00,0x31,0x43,0x41,0x35,0x38,0x34,0x37,0x02,0x44,0x2d,0x53,0x70,0x6f,0x6f,0x6b,0x00,0x79,0x20,0x4d,0x65,0x6c,0x6f,0x64,0x22,0x79,0x23,0x30,0x2e,0x37,0x59,0x24,0x00,0x44,0x65,0x73,0x63,0x72,0x69,0x70,0x00,0x74,0x69,0x6f,0x6e,0x20,0x66,0x6f,0x00,0x72,0x20,0x41,0x6c,0x74,0x65,0x72,0x00,0x6e,0x61,0x74,0x69,0x76,0x65,0x20,0x00,0x50,0x72,0x65,0x73,0x65,0x74,0x20,0x02,0x31,0x28,0x69,0x63,0x6f,0x6e,0x2e,0x28,0x70,0x6e,0x67,0x4a,0x42,0x70,0x00,0x06,0x00,0x17,0x2e,0x62,0x69,0x61,0x73,0x00,0x2e,0x6e,0x6f,0x69,0x73,0x65,0x67,0x18,0x61,0x74,0x65,0x43,0x12,0xf7};
byte SpookyMelodyB[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x0c,0x4f,0x01,0x01,0x34,0x04,0x01,0x00,0x00,0x11,0x4a,0x3f,0x30,0x00,0x00,0x00,0x01,0x11,0x4a,0x3f,0x09,0x00,0x00,0x00,0x2a,0x43,0x6f,0x6d,0x00,0x70,0x72,0x65,0x73,0x73,0x6f,0x72,0x5b,0x43,0x12,0x00,0x11,0x4a,0x3e,0x34,0x5a,0x10,0x65,0x01,0x11,0x4a,0x3e,0x35,0x04,0x55,0x56,0x2d,0x44,0x69,0x73,0x74,0x00,0x6f,0x72,0x74,0x69,0x6f,0x6e,0x54,0x6c,0x53,0x39,0x42,0x13,0x00,0x11,0x4a,0x6a,0x3e,0x0b,0x59,0x67,0x01,0x11,0x4a,0x60,0x3f,0x24,0x73,0x49,0x02,0x11,0x4a,0x14,0x3f,0x18,0x0e,0x74,0x24,0x54,0x77,0x6c,0x69,0x6e,0x43,0x15,0x00,0x11,0x4a,0x68,0x3f,0x1d,0x09,0x79,0x01,0x11,0x4a,0x6e,0x3e,0x7a,0x0d,0x22,0x02,0x11,0x4a,0x62,0x3e,0x68,0x05,0x7c,0x03,0x11,0x4a,0x68,0x3f,0x01,0x4d,0x28,0x04,0x11,0x4a,0x18,0x3f,0x14,0x7a,0x61,0x27,0x55,0x6e,0x00,0x69,0x56,0x69,0x62,0x65,0xf7};
byte SpookyMelodyC[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x0c,0x0d,0x01,0x01,0x5c,0x04,0x02,0x00,0x43,0x13,0x00,0x11,0x49,0x4a,0x3f,0x22,0x78,0x15,0x01,0x11,0x41,0x4a,0x00,0x00,0x00,0x00,0x02,0x11,0x2d,0x4a,0x3e,0x7c,0x55,0x3e,0x2d,0x44,0x00,0x65,0x6c,0x61,0x79,0x45,0x63,0x68,0x60,0x6f,0x46,0x69,0x6c,0x74,0x43,0x15,0x06,0x00,0x11,0x4a,0x3e,0x6d,0x6c,0x20,0x06,0x01,0x11,0x4a,0x3f,0x0e,0x17,0x32,0x06,0x02,0x11,0x4a,0x3f,0x07,0x70,0x1e,0x56,0x03,0x11,0x4a,0x3e,0x1e,0x1c,0x50,0x06,0x04,0x11,0x4a,0x00,0x00,0x00,0x00,0x01,0x2b,0x62,0x69,0x61,0x73,0x2e,0x72,0x60,0x65,0x76,0x65,0x72,0x62,0x43,0x17,0x26,0x00,0x11,0x4a,0x3f,0x76,0x0a,0x05,0x26,0x01,0x11,0x4a,0x3e,0x6d,0x27,0x00,0x66,0x02,0x11,0x4a,0x3e,0x34,0x21,0x0e,0x06,0x03,0x11,0x4a,0x3e,0x66,0x29,0x56,0x66,0x04,0x11,0x4a,0x3e,0x69,0x24,0x24,0x16,0x05,0x11,0x4a,0x3e,0x36,0xf7};
byte SpookyMelodyD[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x0c,0x0a,0x01,0x01,0x48,0x04,0x03,0x0a,0x5b,0x6e,0x06,0x11,0x01,0x4a,0x3f,0x00,0x00,0x00,0x19,0xf7};
byte SpookyMelodyE[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0x1a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x0d,0x7f,0x01,0x38,0x00,0x00,0x7f,0xf7};

    sc3.create_preset(preset16);
    send_receive_bt(sc3);
    send_receive_bt(sc_setpreset7f);

/*   
     SerialBT.write(sc3.buf[0], BLK_SIZE);       delay(100); while (SerialBT.available()) SerialBT.read();
     SerialBT.write(sc3.buf[1], BLK_SIZE);       delay(100); while (SerialBT.available()) SerialBT.read();
     SerialBT.write(sc3.buf[2], BLK_SIZE);       delay(100); while (SerialBT.available()) SerialBT.read();
     SerialBT.write(sc3.buf[3], sc3.last_pos+1); delay(100); while (SerialBT.available()) SerialBT.read();
     SerialBT.write(sc_setpreset7f.buf[0], sc_setpreset7f.last_pos+1); delay(100); while (SerialBT.available()) SerialBT.read();
 */   
//     SerialBT.write)SpookyMelodyA, sizeof(SpookyMelodyA)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SpookyMelodyB, sizeof(SpookyMelodyB)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SpookyMelodyC, sizeof(SpookyMelodyC)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SpookyMelodyD, sizeof(SpookyMelodyD)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SpookyMelodyE, sizeof(SpookyMelodyE)); delay(100); while (SerialBT.available()) SerialBT.read();
     
   
   }
   if (M5.BtnC.wasReleased()) {
      M5.Lcd.println("Fuzzy Jam");
      sc3.create_preset(preset17);
      send_receive_bt(sc3);
      send_receive_bt(sc_setpreset7f);

//   send_preset_request(0);


//      M5.Lcd.clear(TFT_BLACK);
//      M5.Lcd.setCursor(0, 0);

//byte SultansA[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x2f,0x52,0x01,0x01,0x24,0x03,0x00,0x00,0x00,0x7f,0x59,0x24,0x00,0x35,0x30,0x32,0x38,0x65,0x39,0x30,0x00,0x34,0x2d,0x66,0x66,0x30,0x34,0x2d,0x00,0x34,0x38,0x33,0x62,0x2d,0x62,0x34,0x00,0x31,0x61,0x2d,0x38,0x36,0x61,0x38,0x00,0x63,0x64,0x39,0x63,0x34,0x31,0x63,0x02,0x36,0x2e,0x20,0x44,0x69,0x72,0x65,0x00,0x20,0x53,0x74,0x72,0x61,0x69,0x74,0x44,0x73,0x32,0x23,0x30,0x2e,0x37,0x28,0x00,0x56,0x4e,0x20,0x44,0x69,0x73,0x74,0x02,0x20,0x28,0x69,0x63,0x6f,0x6e,0x2e,0x28,0x70,0x6e,0x67,0x4a,0x42,0x70,0x00,0x06,0x00,0x17,0x2e,0x62,0x69,0x61,0x73,0x00,0x2e,0x6e,0x6f,0x69,0x73,0x65,0x67,0x58,0x61,0x74,0x65,0x42,0x13,0x00,0x11,0x41,0x4a,0x3e,0x5f,0x6b,0x0e,0x01,0x11,0x5d,0x4a,0x3d,0x04,0x5f,0x3c,0x02,0x11,0x21,0x4a,0x00,0x00,0x00,0x00,0x28,0x4c,0x00,0x41,0x32,0x41,0x43,0x6f,0xf7};
//byte SultansB[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0xad,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x2f,0x40,0x01,0x01,0x64,0x03,0x01,0x00,0x6d,0x70,0x43,0x13,0x06,0x00,0x11,0x4a,0x00,0x00,0x00,0x00,0x46,0x01,0x11,0x4a,0x3f,0x36,0x6f,0x5e,0x66,0x02,0x11,0x4a,0x3f,0x39,0x0c,0x66,0x01,0x27,0x42,0x6f,0x6f,0x73,0x74,0x65,0x36,0x72,0x42,0x11,0x00,0x11,0x4a,0x3f,0x08,0x44,0x5d,0x17,0x27,0x41,0x44,0x43,0x30,0x6c,0x65,0x61,0x6e,0x43,0x15,0x00,0x23,0x11,0x4a,0x3f,0x3e,0x29,0x20,0x01,0x23,0x11,0x4a,0x3f,0x12,0x02,0x21,0x02,0x3b,0x11,0x4a,0x3e,0x3f,0x46,0x46,0x03,0x3b,0x11,0x4a,0x3e,0x2e,0x2f,0x6c,0x04,0x4b,0x11,0x4a,0x3f,0x00,0x00,0x00,0x27,0x00,0x46,0x6c,0x61,0x6e,0x67,0x65,0x72,0x5b,0x43,0x13,0x00,0x11,0x4a,0x3e,0x77,0x59,0x43,0x05,0x01,0x11,0x4a,0x3e,0x7d,0x18,0x08,0x22,0x02,0x11,0x4a,0x3f,0x2f,0x04,0x68,0x7d,0x2c,0x56,0x69,0x6e,0x74,0x00,0x61,0x67,0x65,0x44,0x65,0xf7};
//byte SultansC[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0x92,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x2f,0x4b,0x01,0x01,0x40,0x03,0x02,0x68,0x6c,0x61,0x79,0x43,0x4d,0x14,0x00,0x11,0x4a,0x3f,0x29,0x7a,0x0c,0x44,0x01,0x11,0x4a,0x3e,0x41,0x70,0x6d,0x13,0x02,0x11,0x4a,0x3e,0x30,0x22,0x2c,0x66,0x03,0x11,0x4a,0x3f,0x00,0x00,0x02,0x00,0x2b,0x62,0x69,0x61,0x73,0x2e,0x40,0x72,0x65,0x76,0x65,0x72,0x62,0x43,0x2d,0x18,0x00,0x11,0x4a,0x3e,0x12,0x49,0x2c,0x25,0x01,0x11,0x4a,0x3e,0x51,0x13,0x2d,0x38,0x02,0x11,0x4a,0x3e,0x14,0x37,0x6d,0x63,0x03,0x11,0x4a,0x3e,0x46,0x51,0x0d,0x03,0x04,0x11,0x4a,0x3f,0x15,0x07,0x0c,0x50,0x05,0x11,0x4a,0x3f,0x26,0x66,0x4c,0x67,0x06,0x11,0x4a,0x3e,0x4c,0x4c,0x2d,0x4d,0x07,0x11,0x4a,0x3f,0x00,0x00,0x00,0x00,0x49,0xf7};
//byte SultansD[]={0x01,0xfe,0x00,0x00,0x53,0xfe,0x1a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x01,0x30,0x7f,0x01,0x38,0x00,0x00,0x7f,0xf7};

//     SerialBT.write(SultansA, sizeof(SultansA)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SultansB, sizeof(SultansB)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SultansC, sizeof(SultansC)); delay(100); while (SerialBT.available()) SerialBT.read();
//     SerialBT.write(SultansD, sizeof(SultansD)); delay(100); while (SerialBT.available()) SerialBT.read();
     
   }  

   if (MIDIUSBH) {
      midi_event();
   }
   
   if (SerialBT.available()) {
      get_data(scr);
//      if (!(scr.buf[0][20]==0x04 && scr.buf[0][6] == 23)) {
      // Not an ACK
         Serial.println();
         Serial.println("Got new unprompted message from Spark");
         scr.dump();
//      }
   }
}
