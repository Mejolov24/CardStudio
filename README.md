# CardStudio 
⚠️ Early development / not stable yet
Kinda outdated readme.md since codebase is actively chaning and has no stable documentation
A Multi app Firmware designed to turn the CardPuter ADV into a Mini Music Studio

## Hardware Limitations
The ESP32S3 is quite fast and can do pretty much anything, but the CardPuter devkit has a small limitation: lack of PSRAM, meaning that we cannot allocate samples quickly in memory.
So, the workaround is to use MMAP (Memory map) Where, instead of having the samples in sd card,  we generate one big .spack wich contains all of the samples. Then its burned into flash and ready to use.
Since we utilize flash, and the StampS3A has 8MB, we are left for around 6MB for sample data, so I recomend using samples btween 8khz to 22khz.

## How to make my own Sample packs?
Its quite simple, you need to use [PCM-MCU-C ](https://github.com/Mejolov24/PCM-MCU-C), wich will take your audio files and pack them in various ways.
Since its a modular tool, you need to use it in a specific way:

* Inside /input create two folders: /0_instruments and /1_percussion
* Inside those folders store your samples numbered from 0-127 using General Midi standard 1, so the file first has the midi sample index and then whatever name you want, for example 000_piano, 016_organ and so on... 

* Select option 1
* 16 BitDepth 
* Choose whatever sample rate you wish, i recomend anything btween 8khz to 22khz.

* Select option 3
* 16 bytes of padding.
* Done, your .spack is at /output 
* Place the .spack into the CardPuter SD Card at /AppData/CardStudio/SamplePacks 

## Features:
- [x] Serial Midi playback
- [x] Serial Oscilloscope View
- [ ] USB Midi playback 
- [ ] SD Midi playback 

## Ecosystem 
This firmware is built around a small ecosystem of music related software/libraries that i made:
* [SynthCore ](https://github.com/Mejolov24/SynthCore)
Freestanding lightweight library for multi voice pcm playback
* [SynthTracer](https://github.com/Mejolov24/SynthTracer)
tool for visualizing serial channel output and sending midi packets.
* [PCM-MCU-C](https://github.com/Mejolov24/PCM-MCU-C)
Sample converter tool for micro controllers.
* [MidiParser](https://github.com/Mejolov24/MidiParser)
Parses Midi and sends a callback with human readable data.
* [M5Config](https://github.com/Mejolov24/M5Config)
UI interface for adjusting variables on M5Stack devices
* [M5SDE](https://github.com/Mejolov24/M5SDE)
UI SD Card Explorer for M5Stack devices
## Internal libraries
* Storage 
FMU (Flash Management Unit)
Controls the flash mmap, sample burning and loading.
* CADVCB (CardPuter ADV Keyboard Callback)
Provides a callback for when a key is pressed, allows easy control.

Copyright (c) 2026 Guillermo Beckers Rival Licensed under the GNU GPLv3
