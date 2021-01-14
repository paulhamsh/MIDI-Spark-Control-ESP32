# MIDI-Spark-Control-ESP32

Control your Positive Grid Spark amp with a MIDI controller, via a M5Stack Core with UBS host shield.

A program for the M5Stack Core (an ESP 32 platform) written in C++

## Installation instructions

## API

Sending to Spark.

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
      
Example: change_hardware_preset (2)
```      

### Change an effect value to a new one (same as moving a know on the Spark amp)
The number for the parameter depends on the effect - for an amp they are:

0 Gain
1 Treble
2 Mid
3 Bass
4 Master

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.

```
void change_effect_parameter (char *pedal, int param, float val);

Example: change_effect_parameter ("RolandJC120", 0, 0.56);
```

### Turn an effect on or off

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.
```
void turn_effect_onoff (char *pedal, char *onoff);

Example: turn_effect_onoff ("DistortionTS9", "Off");
```

### Change an effect to a new one

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.

```
void change_effect (char *pedal1,  char *pedal2);

Example: change_effect ("ChorusAnalog",  "Phaser")
```

### Create preset 

The preset is a C++ structure containing all the data required for a full preset.


```
void create_preset (SparkPreset& preset);

Example: change_effect (preset);

SparkPreset preset{0x0,0x7f,"D8757D67-98EA-4888-86E5-5F1FD96A30C3","Royal Crown","0.7","1-Clean","icon.png",120.000000,{ 
  {"bias.noisegate", true, 3, {0.211230, 0.570997, 0.000000}}, 
  {"Compressor", true, 2, {0.172004, 0.538197}}, 
  {"DistortionTS9", false, 3, {0.703110, 0.278146, 0.689846}}, 
  {"ADClean", true, 5, {0.677083, 0.501099, 0.382828, 0.585946, 0.812231}}, 
  {"ChorusAnalog", true, 4, {0.519976, 0.402152, 0.240642, 0.740579}}, 
  {"DelayMono", true, 5, {0.173729, 0.233051, 0.493579, 0.600000, 1.000000}}, 
  {"bias.reverb", true, 7, {0.688801, 0.392857, 0.461138, 0.693705, 0.488235, 0.466387, 0.300000}} },0xa2 };

```

## Acknowledgements

All inspired by Justin Nelson and the marvellous #tinderbox https://github.com/jrnelson90/tinderboxpedal
With thanks to gdsports on github for the MIDI USB host examples  https://github.com/gdsports/MIDIDump

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/SetupM5StackCore.jpg)

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/Example.jpeg)

