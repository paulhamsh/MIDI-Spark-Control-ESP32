# MIDI-Spark-Control-ESP32

Control your Positive Grid Spark amp with a MIDI controller, via a M5Stack Core with UBS host shield.

A program for the M5Stack Core (an ESP 32 platform) written in C++

## Installation instructions

## API

Sending to Spark

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

Example: change_effect_parameter ("RolandJC120", 0, 0.56) 
```

### Turn an effect on or off

Note: the effect named must be the actual effect in use on the amp otherwise it will not work.
```
void turn_effect_onoff (char *pedal, char *onoff);

Example: turn_effect_onoff ("DistortionTS9", "Off");
```

```
void change_effect (char *pedal1,  char *pedal2);
```

```
void create_preset_json (const char *a_preset);
```

```
void create_preset (SparkPreset& preset);
```

With thanks to gdsports on github for the MIDI USB host examples  https://github.com/gdsports/MIDIDump

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/SetupM5StackCore.jpg)

![Spark Setups](https://github.com/paulhamsh/MIDI-Spark-Control-ESP32/blob/main/diagrams/Example.jpeg)

