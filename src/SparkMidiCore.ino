#include <M5Stack.h>
#include <ArduinoJson.h>
#include "SparkClass.h"
#include <usbh_midi.h>
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32

// Bluetooth vars
#define SPARK_NAME "Spark 40 Audio"
#define MY_NAME "MIDI Spark"

BluetoothSerial SerialBT;

// MIDI vars
USB Usb;
USBH_MIDI MIDIUSBH(&Usb);
uint8_t   UsbHSysEx[1026];
uint8_t   *UsbHSysExPtr;

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

SparkClass sc1, sc2;

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
  
void start_midi() {
  if (Usb.Init() == -1) {
    M5.Lcd.println("USB host init failed");
    while (true); //halt
  }
  UsbHSysExPtr = UsbHSysEx;
}

void midi_event() {
  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t rcode = 0;     //return code
  uint16_t rcvd;
  uint8_t readCount = 0;
  rcode = MIDIUSBH.RecvData( &rcvd, recvBuf);

  //data check
  if (rcode != 0 || rcvd == 0) return;
  if ( recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0 ) {
    return;
  }
  uint8_t *p = recvBuf;
  while (readCount < rcvd)  {
    if (*p == 0 && *(p + 1) == 0) break; //data end

    uint8_t header = *p & 0x0F;
    p++;
    uint8_t chan = (*p & 0x0F) + 1;
    int miditype = (*p & 0xF0);

    M5.Lcd.print(miditype, HEX);
    M5.Lcd.print(" ");
    M5.Lcd.print(p[1], HEX);
    M5.Lcd.print(" ");
    M5.Lcd.print(p[2], HEX);
    M5.Lcd.println();

    p += 3;
    readCount += 4;
  }
}




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
}

void loop() {
   M5.update();
   Usb.Task();
   
   if (M5.BtnA.wasReleased()) {
      sc2.change_hardware_preset(0);
      sc2.dump();
      SerialBT.write(sc2.buf[0], sc2.last_pos+1);
  } 
   if (M5.BtnB.wasReleased()) {
      sc2.change_hardware_preset(1);
      sc2.dump();
      SerialBT.write(sc2.buf[0], sc2.last_pos+1);
   }
   if (M5.BtnC.wasReleased()) {
      M5.Lcd.clear(TFT_BLACK);
      M5.Lcd.setCursor(0, 0);
   }  

   if (MIDIUSBH) {
      midi_event();
   }
}
