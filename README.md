# CardStudio 
A PCM synthesizer for the Cardputer ADV with .spack format support

![thumbnail](https://raw.githubusercontent.com/Mejolov24/CardStudio/refs/heads/main/logo.png)

## Hardware Limitations
The ESP32S3 is quite fast and can do pretty much anything, but the CardPuter has a small limitation: lack of PSRAM, meaning that we cannot allocate samples in memory.
So, the workaround is to use MMAP (Memory map) Where, instead of having the samples in sd card,  we generate one big .spack wich contains all of the samples. Then its burned into flash and ready to use.
Since we utilize flash, and the StampS3A has 8MB, we are left for around ~6MB for sample data (could be more, i havent checked :p)

## How to make my own Sample packs?
Its quite simple, you need to use [PCM-MCU-C ](https://github.com/Mejolov24/PCM-MCU-C), wich will take your audio files and pack them in various ways.
Since its a modular tool, you need to use it in a specific way:

### Settings
* 16 Bit Depth
* 16 bytes of padding
* Choose whatever sample rate you wish, i recomend anything btween 8khz to 22khz for optimal file size and quality

### Conversion
* Inside /input create two folders: /0_instruments and /128_percussion
* Inside those folders store your samples numbered from 0-127 using General Midi standard 1.0, so the file first has the midi sample index and then whatever name you want, for example 000_piano, 016_organ and so on... 

* Select option 1, convert to .pscm

* Select option 4, convert to spack
* Done, your .spack is at /output
* Place the .spack into the CardPuter SD Card at /AppData/CardStudio/SamplePacks 

## How do i convert a .sf2 (SoundFont) to .spack?
it is straightforward, just select option 2, convert .sf2 to .pcm.
and then convert to .spack, the output files will be at /output/sf2

## Features:
- Serial Midi playback
- Serial Oscilloscope View
- Cents tunning
- SD Midi playback 
- All of synthcore capabilities, such as:
    - pitch bend
    - LFO vibrato
    - 32 Voices
    - 16 channels
- Piano roll: *just like synthesia!*
- Virtual Piano: *so you can jam when bored!*

## But, i dont have my own sample packs!
Dont worry, get some at the growing [Open Sample Proyect](https://github.com/Mejolov24/Open-Sample-Proyect)

### TODO:
- [ ] USB Midi playback
- [ ] Tracker (*idk if i will do this, seems hard...*)

## Toolchain
This firmware is built around a small ecosystem of music related software/libraries that i made:
* [SynthCore ](https://github.com/Mejolov24/SynthCore)
Freestanding lightweight library for multi voice pcm playback
* [SynthTracer](https://github.com/Mejolov24/SynthTracer)
tool for visualizing serial channel output and sending midi packets.
* [PCM-MCU-C](https://github.com/Mejolov24/PCM-MCU-C)
Sample converter tool for micro controllers.

## Libraries used
* [MidiParser](https://github.com/Mejolov24/MidiParser)
Parses Midi and sends a callback with human readable data.
* [M5Config](https://github.com/Mejolov24/M5Config)
UI interface for adjusting variables on M5Stack devices
* [M5SDE](https://github.com/Mejolov24/M5SDE)
UI SD Card Explorer for M5Stack devices

### Internal libraries
* [FMU](https://github.com/Mejolov24/CardStudio/tree/main/lib/FMU/src) (Flash Management Unit)
Controls the flash mmap, sample burning and loading.
* [CADVCB](https://github.com/Mejolov24/CardStudio/tree/main/lib/M5CADVKeyCB/src) (Cardputer ADV Keyboard Callback)
Provides a callback for when a key is pressed, allows easy control.*will a library in the future*
* [Synth Wrapper](https://github.com/Mejolov24/CardStudio/tree/main/lib/synth_wrapper)
Midi GM 1.0 translation layer for SynthCore. *will a library in the future*
* [Midi SD](https://github.com/Mejolov24/CardStudio/tree/main/lib/MidiSD)
Translates .mid sd card files onto an array of TimedMidiMessage and also handles playback. *will be separated btween sd and playback*

Copyright (c) 2026 Guillermo Beckers Rival Licensed under the [GNU GPLv3](https://github.com/Mejolov24/CardStudio/blob/main/LICENSE.txt)
