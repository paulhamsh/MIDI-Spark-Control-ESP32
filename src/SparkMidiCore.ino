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

char current[7][STR_LEN]{"bias.noisegate", "LA2AComp", "Booster", "RolandJC120", "Cloner", "VintageDelay", "bias.reverb"};

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

SparkClass sc1, sc2, sc3, scr;
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
   for (i = 0; i < spark_class.last_block; i++) 
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1);
}


// Helper functions to send an acknoweledgement and to send a request for a preset

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

void send_receive_bt(SparkClass& spark_class)
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

*/

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
   int i, j;
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
               send_bt(sc2);
               
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
                 sc2.turn_effect_onoff(current[curr_ind], NoteOn[i].param2);
                 send_bt(sc2);
                 
                 Serial.print("Change effect on/off ");
                 Serial.print(current[curr_ind]);
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
                  send_bt(sc2);
                  // and send the select hardware 7f message too
                  send_bt(sc_setpreset7f);
                 
                  Serial.print("Change preset ");
                  Serial.println(presets[j]->Name);

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
                  sc2.change_effect(current[curr_ind], NoteOn[i].param2);
                  send_bt(sc2);
                 
                  Serial.print("Change effect ");
                  Serial.print(current[curr_ind]);
                  Serial.print(" ");
                  Serial.println(NoteOn[i].param2);
                  // update our current[] map
                  strcpy(current[curr_ind], NoteOn[i].param2);
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
                  sc2.change_effect_parameter(current[curr_ind], CC[i].value, dat2);
                  send_bt(sc2);

                  Serial.print("Change param  ");
                  Serial.print(CC[i].param1);
                  Serial.print(" ");
                  Serial.print(current[curr_ind]);
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
      sc2.change_hardware_preset(0);
      send_bt(sc2);
  } 
   if (M5.BtnB.wasReleased()) {
      sc2.create_preset_json(my_preset);
      sc3.create_preset(preset1);
      send_bt(sc2);
      send_bt(sc_setpreset7f);
   }
   if (M5.BtnC.wasReleased()) {
      M5.Lcd.clear(TFT_BLACK);
      M5.Lcd.setCursor(0, 0);
   }  

   if (MIDIUSBH) {
      midi_event();
   }
   
   if (SerialBT.available()) {
      get_data(scr);
      if (scr.buf[0][20]!=0x04 && scr.buf[0][6] == 23) {
      // Not an ACK
         Serial.println();
         Serial.println("Got new unprompted message from Spark");
         scr.dump();
      }
   }
}
