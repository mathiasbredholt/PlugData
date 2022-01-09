# PlugData
Plugin wrapper around PureData to allow patching in a wide selection of DAWs.

<img width="1024" alt="Screenshot 2021-12-28 at 18 50 02" src="https://user-images.githubusercontent.com/44585538/147593480-34daed21-3f6b-4a11-b1f1-db1d5edb756c.png">
<img width="1024" alt="Screenshot 2021-12-28 at 18 51 48" src="https://user-images.githubusercontent.com/44585538/147593486-8dbae995-bdc7-4ccd-ae6d-2dc4919f2b0d.png">


PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. It can still use some stability fixes, especially for complicated patches. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

```
git clone --recursive https://github.com/timothyschoen/PlugData.git
cd PlugData
mkdir build && cd build
cmake .. (the generator can be specified using -G"Unix Makefiles", -G"XCode" or -G"Visual Studio 16 2019" -A x64)
cmake --build .
```

**Important:**
- Please ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the PlugData repository .
- On Linux OS, Juce framework requires to install dependencies, please refer to [Linux Dependencies.md](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) and use the full command.
- The CMake build system have been tested with *Unix Makefiles*, *XCode* and *Visual Studio 16 2019*.
- Tested with Clang and MSVC

## Credits
- [Camomile](https://github.com/pierreguillot/Camomile) by Pierre Guillot
- [ELSE](https://github.com/porres/pd-else) by Alexandre Torres Porres
- [Pure Data](http://msp.ucsd.edu/software.html) by Miller Puckette and others
- [libpd](http://libpd.cc) by the Peter Brinkmann, Dan Wilcox and others
- [Juce](https://github.com/WeAreROLI/JUCE) by ROLI Ltd.
- [ff_meters](https://github.com/ffAudio/ff_meters) by Foleys Finest Audio
- [MoodyCamel](https://github.com/cameron314/concurrentqueue) by Cameron Desrochers
- [LV2 PlugIn Technology](http://lv2plug.in) by Steve Harris, David Robillard and others
- [VST PlugIn Technology](https://www.steinberg.net/en/company/technologies/vst3.html) by Steinberg Media Technologies
- [Audio Unit PlugIn Technology](https://developer.apple.com/documentation/audiounit) by Apple
- [Juce LV2 interface](http://www.falktx.com) by Filipe Coelho
- [Console](https://github.com/JanosGit) by Janos Buttgereit
- [Multi-Dragger](https://github.com/jcredland/juce-multi-component-dragger) by Jim Credland

## Status
What works:
- Very close to full support for pd (including all GUI objects, undo/redo, copy/paste, saving, loading, console, object properties, drawing functions, audio and MIDI I/O, help files)
- Most ELSE library objects work
- LV2, AU and VST3 formats available, tested on Windows (x64), Mac (ARM/x64) and Linux (ARM/x64), also works as AU MIDI processor for Logic
- Receive 8 DAW parameters by using "receive param1"


Known issues:
- Needs more testing on different systems and DAWs
- There may still be some bugs
- Can be slow for very large patches

Currently PlugData is feature-frozen until all bugs are fixed and the code is cleaned up more.

Future plans:
- Support for GEM
- More GUI conveniences

Please contact me if you wish to contribute, I could use some help!
