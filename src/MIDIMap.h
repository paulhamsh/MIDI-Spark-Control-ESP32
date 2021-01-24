#define MIDI_STR_LEN 40

typedef struct {
   int note;
   char command[MIDI_STR_LEN+1];
   char param1[MIDI_STR_LEN+1];
   char param2[MIDI_STR_LEN+1];
   int value;   
} MIDIMap;

MIDIMap NoteOn[]{ {40, "HwPreset", "", "", 0},
                  {41, "HwPreset", "", "", 1},
                  {42, "HwPreset", "", "", 2},
                  {43, "HwPreset", "", "", 3},
                  
                  {48, "ChangeEffect", "Amp", "RolandJC120", 0},
                  {49, "ChangeEffect", "Amp", "Twin", 0},
                  {50, "ChangeEffect", "Amp", "AC Boost", 0},
                  {51, "ChangeEffect", "Amp", "OrangeAD30", 0},

                  {36, "EfectOnOff", "Drive", "On", 0},
                  {37, "EfectOnOff", "Drive", "Off", 0},
                  {38, "EfectOnOff", "Delay", "On", 0},
                  {39, "EfectOnOff", "Delay", "Off", 0},

                  {44, "ChangePreset", "Silver Ship", "", 0},
                  {45, "ChangePreset", "Sweet Memory", "", 0},
                  {46, "ChangePreset", "Spooky Melody", "", 0},
                  {47, "ChangePreset", "Fuzzy Jam", "", 0}
                };

MIDIMap CC[]{     {21, "ChangeParam", "Amp", "", 0},   //Gain
                  {22, "ChangeParam", "Amp", "", 3},   //Bass
                  {23, "ChangeParam", "Amp", "", 2},   //Mid
                  {24, "ChangeParam", "Amp", "", 1},   //Treble
                  {25, "ChangeParam", "Amp", "", 4},   //Master
                  {26, "ChangeParam", "Mod", "", 0}, 
                  {27, "ChangeParam", "Delay", "", 0}, 
                  {28, "ChangeParam", "Reverb", "", 0}
            };
