# MIDI-Spark-Control-ESP32

Control your Positive Grid Spark amp with a MIDI controller, via a M5Stack Core with UBS host shield.

A program for the M5Stack Core (an ESP 32 platform) written in C++

## What it can do

This has a configurable mapping of MIDI messages to commands to the Spark amp.  
It currently responds to Note On and CC messages.  

On the Spark amp it can:
- change the hardware preset
- send a custom preset (like the app can) - the preset it stored in a C++ structure (see below)
- change an effect (eg change amp from Roland JC120 to Twin)
- turn an effect on or off
- change the value of a parameter (eg gain / treble for an amp)

This is pretty much every change the app can make.   

Whilst this is focussed on MIDI the underlying code and class can be triggered by GPIO with some code changes.

## Pictures

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/SetupM5StackCore.jpg)

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/Example.jpeg)

## Installation instructions

M5 Board installations as per M5 instructions - for Arduino: https://docs.m5stack.com/#/en/arduino/arduino_development    
USB Host: https://github.com/felis/USB_Host_Shield_2.0    
Use the latest version of the USB Host Shield library - earlier ones seem not to work with some MIDI devices (Novation Launchkey 25 as an example)    

## API

### Sending to Spark.

There needs to be a instance of SparkClass.
Then we create a message and send it to the Spark;

```
Example:

SparkClass sc;
sc.change_hardware_preset(2); 
send_receive_bt(sc);

```

### Set new hardware preset (same as pressing the buttons on the Spark amp)
```
void change_hardware_preset (int preset_num); // 0-3
      
Example: sc.change_hardware_preset (2);
```      

### Change an effect value to a new one (for example, increase volume, change treble - same as moving a knob on the Spark amp)
The number for the parameter depends on the effect - for an amp they are:

0 Gain
1 Treble
2 Mid
3 Bass
4 Master

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.

```
void change_effect_parameter (char *pedal, int param, float val);

Example: sc.change_effect_parameter ("RolandJC120", 0, 0.56);  / Change gain to 0.56
```

### Turn an effect on or off

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.
```
void turn_effect_onoff (char *pedal, char *onoff);

Example: sc.turn_effect_onoff ("DistortionTS9", "Off");
```

### Change an effect to a new one

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.

```
void change_effect (char *pedal1,  char *pedal2);

Example: sc.change_effect ("ChorusAnalog",  "Phaser");
```

### Create preset 

The preset is a C++ structure containing all the data required for a full preset.

```
void create_preset (SparkPreset& preset);

Example: sc.change_effect (preset);

SparkPreset preset{0x0,0x7f,"D8757D67-98EA-4888-86E5-5F1FD96A30C3","Royal Crown","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.211230, 0.570997, 0.000000}}, 
  {"Compressor", true, 2, {0.172004, 0.538197}}, 
  {"DistortionTS9", false, 3, {0.703110, 0.278146, 0.689846}}, 
  {"ADClean", true, 5, {0.677083, 0.501099, 0.382828, 0.585946, 0.812231}}, 
  {"ChorusAnalog", true, 4, {0.519976, 0.402152, 0.240642, 0.740579}}, 
  {"DelayMono", true, 5, {0.173729, 0.233051, 0.493579, 0.600000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.688801, 0.392857, 0.461138, 0.693705, 0.488235, 0.466387, 0.300000}} },0xa2 };

```

The preset format explanation is:   

| Description | Example |
|-------------|---------|
| Leave as 0 | 0x00 |
| Preset number - use 0x7f | 0x7f |
| UUID for preset - used an existing one | "D8757D67-98EA-4888-86E5-5F1FD96A30C3" |
| Name for preset - use your own | "Royal Crown" |
| Version - use 0.7 |"0.7" |
| Description - use your own | "1-Clean" |
| Icon - use "icon.png" | "icon.png" |
| BPM - use 120.0 | 120.000000 |
| Effects - see below | |
| Checksum - use anything | 0xa2 | 

Effects are always in the order: 

Noisegate  (always "bias.noisegate"), Compressor, Drive, Amp, Mod, Delay, Reverb (always "bias.reverb")

                                
| Description | Example |
|-------------|---------|
| Name - use a effect name from the list in SparkPresets.h | "Compressor" |
| On / off - true is on | true |
| Number of parameters - needs to be the same as that effect has, can be discovered from other presets in SparkPresets.h | 2 |
| Parameter 0 | 0.172004 |
| Parameter 1 | 0.538197 |
| ..... |  ..... |

## Acknowledgements

All inspired by Justin Nelson and the marvellous #tinderboxpedal https://github.com/jrnelson90/tinderboxpedal    
With thanks to gdsports on github for the MIDI USB host examples  https://github.com/gdsports/MIDIDump    



