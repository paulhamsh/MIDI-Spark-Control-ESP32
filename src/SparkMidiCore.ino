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

#define BACKGROUND TFT_BLACK
#define TEXT_COLOUR TFT_WHITE

#define MIDI   68
#define STATUS 34
#define IN     102
#define OUT    172

#define STD_HEIGHT 30
#define PANEL_HEIGHT 68

BluetoothSerial SerialBT;

// MIDI vars
USB Usb;
USBH_MIDI MIDIUSBH(&Usb);

// Spark vars
SparkClass sc1, sc2, sc3, sc4, scr;
SparkClass sc_setpreset7f;

SparkPreset preset;


// ------------------------------------------------------------------------------------------
// Display routintes

// Display vars
#define DISP_LEN 50
char outstr[DISP_LEN+1];
char instr[DISP_LEN+1];
char statstr[DISP_LEN+1];
char midistr[DISP_LEN+1];

void display_background(const char *title, int y, int height)
{
   int x_pos;

   x_pos = 160 - 3 * strnlen(title, DISP_LEN-1);
//   M5.Lcd.fillRoundRect(0, y, 320, height, 4, BACKGROUND);
   M5.Lcd.drawRoundRect(0, y, 320, height, 4, TEXT_COLOUR);
   M5.Lcd.setCursor(x_pos, y);
   M5.Lcd.print(title);
   M5.Lcd.setCursor(8,y+8);
 
}

void do_backgrounds()
{
   M5.Lcd.setTextColor(TEXT_COLOUR, BACKGROUND);
   M5.Lcd.setTextSize(1);
   display_background("", 0, STD_HEIGHT);
   display_background(" STATUS ",    STATUS, STD_HEIGHT);
   display_background(" MIDI DATA ", MIDI,   STD_HEIGHT);
   display_background(" RECEIVED ",  IN,     PANEL_HEIGHT);
   display_background(" SENT ",      OUT,    PANEL_HEIGHT);         
   M5.Lcd.setTextSize(2); 
   M5.Lcd.setCursor (80, 8);
   M5.Lcd.print("Spark MIDI Core");
}


void display_str(const char *a_str, int y)
{
   char b_str[DISP_LEN+1];

   strncpy(b_str, a_str, 25);
   if (strnlen(a_str,DISP_LEN-1) < 25) strncat(b_str, "                         ", 25-strnlen(a_str,DISP_LEN-1));
   M5.Lcd.setCursor(8,y+8);
   M5.Lcd.print(b_str);


   if (y == STATUS)  Serial.print("STATUS: ");
   else if (y==MIDI) Serial.print("MIDI:   ");
   else if (y==IN)   Serial.print("IN:     ");
   else if (y==OUT)  Serial.print("OUT:    ");
   Serial.println(b_str);
}



// ------------------------------------------------------------------------------------------
// Bluetooth routines
  
void connect_to_spark() {
   bool connected = false;
   int rec;
   
   if (!SerialBT.begin (MY_NAME, true)) {
      display_str("Bluetooth init fail", STATUS);
      while (true);
   }
   
   display_str("Connecting to Spark", STATUS);
   while (!connected) {
      connected = SerialBT.connect(SPARK_NAME);
      if (connected && SerialBT.hasClient()) {
         display_str("Connected to Spark", STATUS);
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
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1);
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

void expect_ack()
{
   int ret;
   int parse_ret;
   int i;

   // this is complex because it is checking the data received - you really only
   // need to do scr.get_data()
   
   display_str("Waiting for Ack", STATUS); 
   ret = scr.get_data();
   if (ret >= 0) {
      display_str("Got responses", STATUS); 
      parse_ret = scr.parse_data();
      if (parse_ret < 0) 
         display_str("Parse failed", STATUS);
      else
         if (scr.num_messages > 0)
            for (i = 0; i<scr.num_messages; i++)
               if (scr.messages[i].cmd == 0x04)
                  display_str("Got ack", STATUS);
               else {
                  snprintf(statstr, DISP_LEN-1, "Got something %2.2X %2.2X", scr.messages[i].cmd, scr.messages[i].sub_cmd);
                  display_str(statstr, STATUS);   
               }
   }     
   else
      display_str("Bad block received", STATUS); 
}


void send_receive_bt(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      // read the ACK
      expect_ack();
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 
   // read the ACK
   expect_ack();
}


// ------------------------------------------------------------------------------------------
// MIDI routines
  
void start_midi() {
   if (Usb.Init() == -1) {
      display_str("USB host init failed", STATUS);
      while (true); //halt
   }
}

int get_current_index(char *type) {
   int i;

   if      (!strncmp(type, "Gate", MIDI_STR_LEN-1))   i = 0;
   else if (!strncmp(type, "Comp", MIDI_STR_LEN-1))   i = 1;
   else if (!strncmp(type, "Drive", MIDI_STR_LEN-1))  i = 2;           
   else if (!strncmp(type, "Amp", MIDI_STR_LEN-1))    i = 3;
   else if (!strncmp(type, "Mod", MIDI_STR_LEN-1))    i = 4;
   else if (!strncmp(type, "Delay", MIDI_STR_LEN-1))  i = 5;
   else if (!strncmp(type, "Reverb", MIDI_STR_LEN-1)) i = 6;
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

      snprintf(midistr, DISP_LEN-1, "%02Xh %d %d", miditype, dat1, dat2);
      display_str(midistr, MIDI);

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
      
      // Handle Note On
      found_midi_map = false;
      if (miditype == 0x90) {
         for (i = 0; i < num_noteon_map; i++)
            if (NoteOn[i].note == dat1) {
               found_midi_map = true;
               break;
            }

         if (found_midi_map) {   
            if (!strncmp(NoteOn[i].command, "HwPreset", MIDI_STR_LEN-1)) {
               sc2.change_hardware_preset(NoteOn[i].value);
               send_receive_bt(sc2);

               // the effect list details are updated in the bluetooth receiver code - this request triggers it
               send_preset_request(NoteOn[i].value);
               
               snprintf(outstr, DISP_LEN-1, "Hardware preset %d", NoteOn[i].value);
               display_str(outstr, OUT);
            }
            else if (!strncmp(NoteOn[i].command, "EfectOnOff", MIDI_STR_LEN-1)) {
               // find the current[] effect of this type
               curr_ind = get_current_index(NoteOn[i].param1);
               if (curr_ind >= 0) {
                  sc2.turn_effect_onoff(current_effects[curr_ind], NoteOn[i].param2);
                  // this is interesting - there is an ACK if the effect is actually CHANGED, but if the same state
                  // is sent twice in a row there is no ACK - so not requesting to wait for one here
                  send_bt(sc2);

                  snprintf(outstr, DISP_LEN-1, "%s %s", current_effects[curr_ind], NoteOn[i].param2);
                  display_str(outstr, OUT);
              } 
            }
            else if (!strncmp(NoteOn[i].command, "ChangePreset", MIDI_STR_LEN-1)) {
               // find the current[] effect of this type
               found_preset = false;

               for (j = 0; j < num_presets; j++) {
                  if (!strncmp(presets[j]->Name, NoteOn[i].param1, MIDI_STR_LEN-1)) {
                     found_preset = true;
                     break;
                  }
               }
               if (found_preset) {
                  sc2.create_preset(*presets[j]);
                  send_receive_bt(sc2);
                  // and send the select hardware 7f message too
                  send_receive_bt(sc_setpreset7f);
                  
                  snprintf(outstr, DISP_LEN-1, "Preset %s", presets[j]->Name);
                  display_str(outstr, OUT);
                                   
                  for (k=0; k<7; k++) strncpy(current_effects[k], presets[j]->effects[k].EffectName, STR_LEN-1);
               }
            }
            else if (!strncmp(NoteOn[i].command, "ChangeEffect", MIDI_STR_LEN-1)) {
               // find the current[] effect of this type
               curr_ind = get_current_index(NoteOn[i].param1);
               if (curr_ind >= 0) {
                  sc2.change_effect(current_effects[curr_ind], NoteOn[i].param2);
                  send_receive_bt(sc2);
 
                  // update our current[] map
                  strncpy(current_effects[curr_ind], NoteOn[i].param2, STR_LEN-1);

                  snprintf(outstr, DISP_LEN-1, "%s -> %s", NoteOn[i].param1, NoteOn[i].param2);
                  display_str(outstr, OUT);
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
            if (!strncmp(CC[i].command, "ChangeParam", MIDI_STR_LEN-1)) {
               curr_ind = get_current_index(CC[i].param1);
               if (curr_ind >= 0) {
                  sc2.change_effect_parameter(current_effects[curr_ind], CC[i].value, float(dat2) / 127);
                  send_bt(sc2);

                  snprintf(outstr, DISP_LEN-1, "%s %d %0.2f", current_effects[curr_ind], CC[i].value, float(dat2) / 127);
                  display_str(outstr, OUT);
               }
            }  
         }                   
      }
   }
}



// ------------------------------------------------------------------------------------------

int i, j, k;
int ret, parse_ret;
int param;
float val;
int cmd, sub_cmd;
char a_str[STR_LEN+1];
char b_str[STR_LEN+1];
int pres[]{0,1,2,3,0x7f,0x80,0x81,0x82,0x83};

void setup() {
   M5.begin();
   M5.Power.begin();
   M5.Lcd.fillScreen(BACKGROUND);
   do_backgrounds();

   display_str("Started",      STATUS);
   display_str("Nothing out",  OUT);
   display_str("Nothing in",   IN);
   display_str("No MIDI data", MIDI);
  
   Serial.begin(115200);
   Serial.println("Spark Class Core");

   start_midi();
   connect_to_spark();

   // set up the change to 7f message for when we send a full preset
   sc_setpreset7f.change_hardware_preset(0x7f);

   k = 0;
 
}

void loop() {
   M5.update();
   Usb.Task();
      
   if (M5.BtnA.wasReleased()) {
      display_str("Spooky Melody to 0x7f", OUT);
      preset16.preset_num=0x7f;
      sc3.create_preset(preset16);
      send_receive_bt(sc3);
      send_receive_bt(sc_setpreset7f);      
   }
    
   if (M5.BtnB.wasReleased()) {
      send_preset_request(pres[k]);
      k++;
      if (k>8) k=0;
   }
   
   if (M5.BtnC.wasReleased()) {
      display_str("Request preset 0x7f", OUT);

      send_preset_request(0x7f);
   }  

   if (MIDIUSBH) {
      midi_event();
   }
   
   if (SerialBT.available()) {
      display_str("Getting data from Spark", STATUS);  
      ret = scr.get_data();
      if (ret < 0) {
         snprintf(statstr, DISP_LEN-1, "Corrupted data %d", ret);
         display_str(statstr, STATUS); 
      }
      if (ret >=0 ) {
         display_str("Parsing the data", STATUS);       
         parse_ret = scr.parse_data();
         if (parse_ret >= 0) { 
            if (scr.num_messages > 0) {
               snprintf(statstr, DISP_LEN-1, "Processing %d messages", scr.num_messages);
               display_str(statstr, STATUS);   
            }
            for (i=0; i<scr.num_messages; i++) {
               cmd = scr.messages[i].cmd;
               sub_cmd = scr.messages[i].sub_cmd;
               if (cmd == 0x03 && sub_cmd == 0x37) {
                  scr.get_effect_parameter(i, a_str, &param, &val);
             
                  snprintf(instr, DISP_LEN-1, "%s %d %0.2f", a_str, param, val);
                  display_str(instr, IN);             
               }
               else if (cmd == 0x03 && sub_cmd == 0x06) {
                  // it can only be an amp change if received from the Spark
                  scr.get_effect_change(i, a_str, b_str);
                  strncpy(current_effects[3], b_str, STR_LEN-1);

                  snprintf(instr, DISP_LEN-1, "-> %s", b_str);
                  display_str(instr, IN);
         
               }
               else if (cmd == 0x03 && sub_cmd == 0x01) {
               scr.get_preset(i, &preset);
               // update the current effects list
               for (j=0; j<7; j++) strncpy(current_effects[j], preset.effects[j].EffectName, STR_LEN-1);

               snprintf(instr, DISP_LEN-1, "Preset: %s", preset.Name);
               display_str(instr, IN);
               }
               else {
                  snprintf(instr, DISP_LEN-1, "Command %2.2X %2.2X", scr.messages[i].cmd, scr.messages[i].sub_cmd);
                  display_str(instr, IN);
               }
            }
         }
         display_str("Done", STATUS);
      }
   }
}
