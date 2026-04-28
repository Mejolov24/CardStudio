# CardStudio 
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
- [ ] Serial Midi playback
- [ ] Serial Oscilloscope View
- [ ] USB Midi playback 
- [ ] SD Midi playback 

## Ecosystem 
This firmware is built around a small ecosystem of music related software/libraries:
* [SynthCore ](https://github.com/Mejolov24/SynthCore)
Freestanding lightweight C++ library for multi voice pcm playback

* SynthTrace
tool for visualizing SynthCore serial channel output.
* [PCM-MCU-C](https://github.com/Mejolov24/PCM-MCU-C)  Sample converter tool for micro controllers.

## Internal workflow
The app is split onto 4 Core libraries:
* Audio engine
powered by [SynthCore](https://github.com/Mejolov24/SynthCore),  The one in charge for the audio processing, handling real-time high quality DSP

* Storage 
FMU Flash Management Unit)
Controls the flash mmap, sample burning and loading.
* CADVCB (CardPuter ADV Keyboard Callback)
Provides a callback for when a key is pressed, allows easy control.
* MidiParser
Parses Midi and sends a callback with human readable data.

