#include <M5Stack.h>
#include <ArduinoJson.h>
#include "SparkClass.h"
#include "SparkPresets.h"

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


// Spark vars
SparkClass sc1, sc2, sc3, sc4, scr;
SparkClass sc_setpreset7f;

SparkPreset preset;


// ------------------------------------------------------------------------------------------
// Display routintes

// Display vars
#define DISP_LEN 50
char outstr[DISP_LEN];
char instr[DISP_LEN];
char statstr[DISP_LEN];
int bar_pos;
unsigned long bar_count;

void display_background(const char *title, int y, int height)
{
   int x_pos;

   x_pos = 160 - 3 * strlen(title);
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
   display_background(" STATUS ",    STATUS, PANEL_HEIGHT);
   display_background(" RECEIVED ",  IN,     PANEL_HEIGHT);
   display_background(" SENT ",      OUT,    PANEL_HEIGHT);         
   M5.Lcd.setTextSize(2); 
   M5.Lcd.setCursor (45, 8);
   M5.Lcd.print("Spark Pedal Override");
   bar_pos=0;
   bar_count = millis();
   
}

void display_bar()
{
   if (millis() - bar_count > 400) {
      bar_count = millis();
      M5.Lcd.fillRoundRect(15 + bar_pos*30, STATUS + STD_HEIGHT + 10 , 15, 15, 4, BACKGROUND);
      bar_pos ++;
      bar_pos %= 10;
      M5.Lcd.fillRoundRect(15 + bar_pos*30, STATUS + STD_HEIGHT + 10, 15, 15, 4, TEXT_COLOUR);
   }
}

void display_str(const char *a_str, int y)
{
   char b_str[30];

   strncpy(b_str, a_str, 25);
   if (strlen(a_str) < 25) strncat(b_str, "                         ", 25-strlen(a_str));
   M5.Lcd.setCursor(8,y+8);
   M5.Lcd.print(b_str);
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


void send_receive_bt(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) {
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      // read the ACK
//      delay(100);
      rec = scr.get_data();
      if (rec == -1) Serial.println("WAITING FOR ACK, GOT AN ERROR"); 
//      while (SerialBT.available()) SerialBT.read(); 
   }
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 
   
   // read the ACK
//   delay(100);
      rec = scr.get_data();
      if (rec == -1) Serial.println("WAITING FOR ACK, GOT AN ERROR"); 
//   while (SerialBT.available()) SerialBT.read(); 
}


// ------------------------------------------------------------------------------------------

int i, j, p;
int pres;

int cmd, sub_cmd;
char a_str[STR_LEN];
char b_str[STR_LEN];
int param;
float val;

void setup() {
   M5.begin();
   M5.Power.begin();
   M5.Lcd.fillScreen(BACKGROUND);
   do_backgrounds();

   display_str("Started",      STATUS);
   display_str("Nothing out",  OUT);
   display_str("Nothing in",   IN);

   connect_to_spark();

   // set up the change to 7f message for when we send a full preset
   sc_setpreset7f.change_hardware_preset(0x7f);
}

void loop() {
   int av;
   
   M5.update();
//   display_bar();

   if (M5.BtnA.wasReleased()) {
      display_str("STILL ALIVE", IN);
   }

   if (!SerialBT.hasClient()) {
      display_str("Lost connection",      STATUS);
      Serial.println("CONNECTION LOST");
      while (true);
   }

   delay(10);
   if (SerialBT.available()) {
      if (scr.get_data() != -1) {
         scr.parse_data();
    
         for (i=0; i<scr.num_messages; i++) {
            cmd = scr.messages[i].cmd;
            sub_cmd = scr.messages[i].sub_cmd;


            if (cmd == 0x03 && sub_cmd == 0x01) {
               scr.get_preset(i, &preset);
               snprintf(instr, sizeof(instr), "Preset: %s", preset.Name);
               display_str(instr, IN);
            }
            else if (cmd == 0x03 && sub_cmd == 0x37) {
               scr.get_effect_parameter(i, a_str, &param, &val);
       
               snprintf(instr, sizeof(instr), "%s %d %0.2f", a_str, param, val);
               display_str(instr, IN);  
            }
            else if (cmd == 0x03 && sub_cmd == 0x06) {
               // it can only be an amp change if received from the Spark
               scr.get_effect_change(i, a_str, b_str);
            
               snprintf(instr, sizeof(instr), "-> %s", b_str);
               display_str(instr, IN);
            
//               for (p=0; p<5;p++) {
//                  sc2.change_effect_parameter(b_str, p, 0.1);
//                  send_bt(sc2);
//               }
            
               if      (!strcmp(b_str, "FatAcousticV2")) pres = 16;
               else if (!strcmp(b_str, "GK800")) pres = 17;
               else if (!strcmp(b_str, "Twin")) pres = 3;
               else if (!strcmp(b_str, "TwoStoneSP50")) pres = 12;
               else if (!strcmp(b_str, "OverDrivenJM45")) pres = 5; 
               else if (!strcmp(b_str, "AmericanHighGain")) pres = 22;
               else if (!strcmp(b_str, "EVH")) pres = 7;
                                               
               sc2.create_preset(*presets[pres]);
               send_receive_bt(sc2);
               send_receive_bt(sc_setpreset7f);

               snprintf(outstr, sizeof(outstr), "Preset: %s", presets[pres]->Name);
               display_str(outstr, OUT);

//              send_preset_request(0x7f);
            }
            else {
//               snprintf(instr, sizeof(instr), "Command %2X %2X", scr.messages[i].cmd, scr.messages[i].sub_cmd);
//               display_str(instr, IN);
            }
         }
      }
   }
}
