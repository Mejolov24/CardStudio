# CardStudio RoadMap #
This is a roadmap listing all my goals for this proyect, current status, things to do, additional features and more.

## App plans ##
- [] Freeplay
- [] MidiPlayer
- [] MiniDaw

## Folder Structure ##

Sd card:

AppData/CardStudio/SamplePacks/...

At this directory, the user will provide the sample packs, wich is simply a folder each follows this simple structure for the instruments and percussion .pcm samples:

SamplePacks/YourSamplePack/instruments
SamplePacks/YourSamplePack/Percussion

## Sample Format ##

Converted via PCM-MCU-C:
My own Sample format called ".pcm"
Wich is almost exactly like .wav, but with a fixed header in this extructure, using very little processing power since its uncompressed, containing only the necessary things for a sample, wheter its a sound effect, a voice, or an instrument like the violin or the piano.

Uint8_t BitDepth;  uint16_t SampleRate; bool Looping; LoopA; LoopB;

## Core Systems ##

- [] SynthCore Helpers:
For handling multiple channels, sample swaps and loading.

Channel_Instumets[16]
Array of SampleData to provide a clear sample selection

SetChannelInstrument(uint8_t channel, uint8_t SID)
Called Upon a Midi event like SetChannel, or other modes like freeplay.
All we do is set the recived SID and store it at the specified channel, this is a lookup table for SID.

GetSampleFromChannel(uint8_t Channel, bool is_percussion)

As it name says we simply get the SampleData Struct from the specified channel SID.

We first pass it through the GetSIDOrfallback()
Then we grab the returned ID and grab the SampleData from Instruments or Percussion arrays, corresponding to the is_percussion bool.

- [] ## MidiHCB ( Midi Humanizer) ##
This class will be in the handling of turning a Midi raw value like 0x90 into a readable value like NoteOn, Channel 10.

We are going to use a callback system for managing the Routing to SynthCore, with some predictable functions like

MidiHM.setup() (mode set, like Serial, or Translator, feeding via another function)
MidiHM.feedData(uint8_t, uint8_t)
MidiHM.loop() (only used at serial mode)

- [] GetSIDOrfallback()
This function takes the SID and returns a valid SID.
Why is this needed:
Because this world is not perfect and we don always have all the instruments we ask for, and playing silence would be a bad for the user experience,
so we use this function to do what we can.

This is the function Workflow:
First, we check if the requested sample exists, if it does we return the same sample.

If it doesnt we search for all nearest sample in the current category, if there is one, we return its SID. (Fallback 1)

If there isnt a neighbor, we return the SID 255 wich is EmbeddedSineWave (Fallback 2)

This way we avoid silence in a smart way, and enhance user experience due to the tight storage conditions.


- [] ## SMU (Storage Management  Unit) ##
This will be a simple wrapper of the used storage option, in this case i will use LittleFS.

Here's why:
due to the very small amount of RAM and the lack of Built-in PSRAM for the CardPuter ADV
I needed a proper way to quickly access samples since sd card is slow, in this specific case littleFS is perfect.

Functions:
- [] LoadSamples()
This function is going to take every sample loaded into LittleFS and store them into the instrument or percussion arrays via a SampleData struct.

- [] UploadSample()
This function needs as an argument a directory for the specific sample pack to load, this will remove all stored samples every time to combat tight storage.

-[] ClearStorage()
Deletes all of the stored samples.


- [] ### SynthCore ###
Synth Core is the pure math engine that handles the processing of voices and instruments, nothing is hardware specific.

 - [] VoiceHandling
  - [] Voice Stealing
  - [] Voice ID

 - [x] Helper functions
  - [x] AddVoice()
  - [x] RemoveVoice()
  - [x] StepAudio()
  - [x] UpdateAudioBuffer[]
  - [x] SetChannelPitchBend()

- [] Struct System for managing samples and configuration, will behave as following:


Struct SampleData{ *sample; *sample len: bool looping; loopA; LoopB;}

Struct NoteData{*SampleData; note; volume;}

Struct EngineNoteData{*SampleData; *NoteData; step; pitch_bend; index;}
(Private)


Then, those arrays will be used this way:
When the user wants to create a voice he must : Provide sample data.

Not necessary but heavily recomended to provide NoteData, wich is simply the configuration.

Upon Voice creation user will use a helper as stated above.
its recomended to import the two structs of SampleData and NoteData for easier management.

EngineNoteData is managed Internally to prevent accidental configuration of engine only data, contains the two public Structs.

So, when calling addVoice, it will need two arguments, Sample data struct and Note data struct.

Since User has a Struct for SampleData, a useful way of handling them is by having an array.
Since we are making this engine based on the midi 1.0 standard, we recomend allocating an array of 128 elements for both instruments and percussion, since anyways more than that isnt needed, but the safeguard is.

