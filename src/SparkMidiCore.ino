#include <M5Stack.h>
#include <ArduinoJson.h>
#include "SparkClass.h"
#include "SparkPresets.h"
#include <usbh_midi.h>
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32

// Bluetooth vars
#define SPARK_NAME "Spark 40 Audio"
#define MY_NAME "MIDI Spark"

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

SparkClass sc1, sc2, sc_setpreset7f;

// ------------------------------------------------------------------------------------------
// Bluetooth routines
  
void connect_to_spark() {
   bool connected = false;
   
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
}

void send_bt(SparkClass& spark_class)
{
   int i;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) 
      SerialBT.write(spark_class.buf[i], BLK_SIZE);
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1);
}

void send_receive_bt(SparkClass& spark_class)
{
   int i;
   int rec;

   // if multiple blocks then send all but the last - the for loop should only run if last_block > 0
   for (i = 0; i < spark_class.last_block; i++) 
      SerialBT.write(spark_class.buf[i], BLK_SIZE);

   while (SerialBT.available())
      rec = SerialBT.read();
      
   // and send the last one      
   SerialBT.write(spark_class.buf[spark_class.last_block], spark_class.last_pos+1); 

   while (SerialBT.available())
      rec = SerialBT.read(); 
}

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
*/
}

// ------------------------------------------------------------------------------------------
// MIDI routines
  
void start_midi() {
   if (Usb.Init() == -1) {
      M5.Lcd.println("USB host init failed");
      while (true); //halt
   }
}

void midi_event() {
   uint8_t receive_buf[MIDI_EVENT_PACKET_SIZE];
   uint16_t received;

   uint8_t *p;
   
   int return_code = 0;     //return code
   int count = 0;

   int header, chan, miditype, dat1, dat2;

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

      if (miditype == 0x90)
         if (dat1 >= 40 && dat1 <= 43) {
            sc2.change_hardware_preset(dat1 - 40);
            SerialBT.write(sc2.buf[0], sc2.last_pos+1);
         }
      else if (miditype == 0xB0)
         if (dat1 >= 21 && dat1 <= 25) {
            //sc2.change_parameter ("Roland JC120", dat1 - 21, dat2);
            //SerialBT.write(sc2.buf[0], sc2.last_pos+1);                       
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
      sc2.create_preset(my_preset);
      send_receive_bt(sc2);
      send_receive_bt(sc_setpreset7f);
   }
   if (M5.BtnC.wasReleased()) {
      M5.Lcd.clear(TFT_BLACK);
      M5.Lcd.setCursor(0, 0);
   }  

   if (MIDIUSBH) {
      midi_event();
   }
}
