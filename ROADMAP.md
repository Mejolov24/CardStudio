# CardStudio RoadMap #
This is a roadmap listing all my goals for this proyect, current status, things to do and additional features.

## Future TODO ##
- [ ] Add USB OTG midi support.
- [ ] allow for 8 bit samples


## Current TODO ###
- [ ] update synth core to add planned changes.
re organize SynthCore structs.

- [ ] create PCM-MCU-C branch called packer

make that if you dont input anything under filepath, grab all files from /input

- [ ] (SynthCore) move channel specific configuration into a ChannelData struct for less nesting and simpler code, and a unified setter.

- [ ] (SynthCore) Move into VID (Voice ID) for voice management

- [ ] SortedVID
an array of VoicesID soterrada in time, oldest sample is the first one, newest is the    
rightmost one.


Now addVoice will remove voices via RemoveVoice[VID]

- [ ] Create a helper for removing voices in specific channels, called RemoveVoiceFromCh[]
wich will search for the matching voice and channel inside SortedVID from rigth to left, and delete it, then return in order to only delete one.

- [ ] implement voice stealing.
when the voices are full, delete the oldest VID at SortedVID(first element), and store the voice there.


- [ ] (SynthCore) New sample end/loop handling, I will do the following:

IMPLEMENT ALL OF THIS INSIDE REMOVEVOICE()

If we stop the voice, and we arent sustaining on this channel and we arent a looping voice, stop immediately, 
if we are a looping voice, continue until sample ends.

if we stop the voice and we are sustaining but we arent a looping voice, continue the sample until it ends, raise "releasing" bool to true

if we are a looping voice, then ignore raise the "released" bool to true.


We check at the RemoveVoice() if we have the "released" flag, if we do and we arent at sustain, we do nothing and remove the voice at the voice synthesis stage for when the voice stops.

if we are sustaining and we try to remove the voice and we are a looping voice, then we dont remove it, but raise the "releasing" flag to true.

at voice synthsis if we have "releasing" flag set to true, we wait until sample end and then kill the voice.

 


## App plans ##
- [ ] Freeplay
- [ ] MidiPlayer
- [ ] MiniDaw

## Folder Structure ##

Sd card:

AppData/CardStudio/SamplePacks/...

At this directory, the user will provide the sample packs, wich is simply a folder each follows this simple structure for the instruments and percussion .pcm samples:

SamplePacks/YourSamplePack/instruments
SamplePacks/YourSamplePack/Percussion


## Core Systems ##

- [ ] SynthCore Helpers:
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

- [ ] ## MidiParser ##
This class will be in the handling of turning a Midi raw value like 0x90 into a readable value like NoteOn, Channel 10.

We are going to use a callback system for managing the Routing to SynthCore, with some predictable functions like

MidiParser.feedData(uint8_t, uint8_t) (Manual)
MidiParser.SeialLoop() (Auto, Via Serial using my own script or Hairless midi serial)

- [ ] GetSIDOrfallback()
This function takes the SID and returns a valid SID.
Why is this needed:
Because this world is not perfect and we don always have all the instruments we ask for, and playing silence would be a bad for the user experience,
so we use this function to do what we can.

This is the function Workflow:
First, we check if the requested sample exists, if it does we return the same sample.

If it doesnt we search for all nearest sample in the current category, if there is one, we return its SID. (Fallback 1)

If there isnt a neighbor, we return the SID 255 wich is EmbeddedSineWave (Fallback 2)

This way we avoid silence in a smart way, and enhance user experience due to the tight storage conditions.


- [ ] ## SMU (Storage Management  Unit) ##

Due to the little amount of RAM, we will pack all samples into a .bin file, then load it at runtime using mmap, similar to what gamestation does.
this file will be generated with a python script.

SampleData structure"

Uint8_t BitDepth;  uint16_t SampleRate; bool Looping; LoopA; LoopB;

Functions:
- [ ] LoadSamplePack()
This will read the currently stored samples pack, and make it accesible.

- [ ] UploadSamplePack()
This function needs as an argument a directory for the specific sample pack to load, this will remove all stored samples every time to combat tight storage.

- [ ] ClearStorage()
Deletes the stored samples pack.


- [ ] ## SynthCore ##
Synth Core is the pure math engine that handles the processing of voices and instruments, nothing is hardware specific.

 - [ ] VoiceHandling
  - [ ] Voice Stealing
  - [ ] Voice ID

 - [x] Helper functions
  - [x] AddVoice()
  - [x] RemoveVoice()
  - [x] StepAudio()
  - [x] UpdateAudioBuffer[]
  - [x] SetChannelPitchBend()

- [ ] Struct System for managing samples and configuration, will behave as following:


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

