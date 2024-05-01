# cc3dsfs

cc3dsfs is a multi-platform capture and display program for [3dscapture's](https://3dscapture.com/) N3DSXL capture board written in C++.
The main goal is to offer the ability to use the Capture Card with a TV, via fullscreen mode.

#### Features

- Performance-focused design, with low latency for both audio and video (measured to oscillate between 1 and 2 frames).
- Option to split the screens in separate windows, to address them separately.
- Make your game run in fullscreen mode. If you own multiple displays, you can even use one per-window.
- Many builtin crop options for the screens.
- Many other settings, explained in the [Controls](####Controls) section.

_Note: On 3DS, DS, GBA, GBC and GB games boot in scaled resolution mode by default. Holding START or SELECT while launching these games will boot in native resolution mode._
_Note: Make sure the 3DS audio is not set to Surround._

#### Dependencies

cc3dsfs has three build dependencies: CMake, g++ and git.
Make sure all are installed.
On MacOS, [Homebrew](https://brew.sh/) can be used to install both CMake and git. An automatic popup should appear to install g++ at Compile time.

cc3dsfs has two library dependencies: [FTDI's D3XX driver](https://ftdichip.com/drivers/d3xx-drivers/) and [SFML](https://www.sfml-dev.org/).
Both should get downloaded automatically via CMake during the building process.

Linux users will also need to install the SFML dependencies. Different distributions will require slightly different processes.
Below, the command for Debian-based distributions, which also lists the required libraries.

```
sudo apt update
sudo apt install \
    libxrandr-dev \
    libxcursor-dev \
    libudev-dev \
    libopenal-dev \
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libdrm-dev \
    libgbm-dev \
    libfreetype-dev
```

#### Compile and Install

To compile the program, assuming CMake, git and g++ are installed on the system, this is the command which should be launched:

```
cmake -B build ; cmake --build build --config Release
```

This will download both FTD3XX and SFML, which may take a while during the first execution of the command. Later runs should be much faster.
On MacOS, you may also be prompted to install the Apple Command Line Developer Tools first.

#### Controls

- __S key__: Swaps between Split mode and Joint mode which splits the screens into separate windows or joins them into a single window respectively.
- __A key__: Ensures all the available windows show the same frame. If you're having framerate issues, turning this setting on/off could help.
- __C key__: Cycles to the next cropping mode for the focused window. The currently supported cropping modes are for 3DS, 16:10 DS, scaled DS, native DS, scaled GBA, native GBA, scaled GB, native GB, native SNES and native NES respectively.
- __B key__: Toggles blurring on/off for the focused window. This is only noticeable at scales of 1.5x or greater.
- __V key__: Toggles VSync on/off for the focused window. VSync prevents screen tearing. However, at low refresh rates (60 hz), it may significantly increase image delay.
- __T key__: Moves the relative position of the bottom screen inside of the joint mode window clockwise.
- __F key__: Toggles Fullscreen mode on/off. Only guaranteed to work on the primary monitor. When using certain setups, it also works for multiple monitors.
- __8 key__: Rotates the screen(s) of the focused window 90 degrees counterclockwise.
- __9 key__: Rotates the screen(s) of the focused window 90 degrees clockwise.
- __H key__: Rotates the top screen of the focused window 90 degrees counterclockwise.
- __J key__: Rotates the top screen of the focused window 90 degrees clockwise.
- __K key__: Rotates the bottom screen of the focused window 90 degrees counterclockwise.
- __L key__: Rotates the bottom screen of the focused window 90 degrees clockwise.
- __- key__: Decrements the scaling by 0.5x for the non-Fullscreen focused window. 1.0x is the minimum.
- __0 key__: Increments the scaling by 0.5x for the non-Fullscreen focused window. 45.0x is the maximum.
- __Y key__: In Joint Fullscreen mode, increases the size of the top screen.
- __U key__: In Joint Fullscreen mode, increases the size of the bottom screen.
- __4 key__: In Fullscreen mode, changes the X distance from the border between 0, 1/2 Max and Max.
- __5 key__: In Fullscreen mode, changes the Y distance from the border between 0, 1/2 Max and Max.
- __6 key__: In Joint mode, changes the position of the smaller screen relative to the bigger one between 0, 1/2 Max and Max.
- __7 key__: In Joint Fullscreen mode, changes the distance from the upper screen between 0, 1/2 Max and Max.
- __I key__: Toggles BFI (Motion Blur Reduction) on/off. Best used on 120+hz monitors. DO NOT USE THIS IF YOU'RE AT RISK OF SEIZURES!!!
- __Z key__: Decreases the Menu Scaling multiplier by 0.1. 0.3 is the minimum.
- __X key__: Increases the Menu Scaling multiplier by 0.1. 5.0 is the maximum.
- __M key__: Toggles mute on/off.
- __, key__: Decrements the volume by 5 units. 0 is the minimum.
- __. key__: Increments the volume by 5 units. 100 is the maximum.
- __O key__: Open/Close connection to the 3DS.
- __F1 - F4 keys__: Loads from layouts 1 through 4 respectively.
- __F5 - F8 keys__: Saves to layouts 1 through 4 respectively.
- __Esc key__: Properly quits the program.

_Note: The volume is independent of the actual volume level set with the physical slider on the console._

#### Settings

When starting the program for the first time, a message indicating a load failure for the cc3dsfs.cfg file will be displayed, and the same will occur when attempting to load from any given layout file if it hasn't been saved to before. These files must be created by the program first before they can be loaded from. The program saves its current configuration to the cc3dsfs.cfg file when the program is successfully closed, creating the file if it doesn't already exist, and loads from it every time at startup.

Just as well, the current configuration can be saved to any of the four layout files at any time using keys F5 through F8, creating the given file if it doesn't already exist, which can then be loaded from at any time using keys F1 through F4 respectively. Changing the configuration after a layout is loaded will not overwrite it unless the respective save key is pressed after the changes are made.

#### Notes

- At startup, the audio may be unstable. It should fix itself, if you give it enough time.
- If, at first, the connection to the 3DS fails, reconnect the 3DS and then try again. If that also doesn't work, try restarting the program. If that also doesn't work, try restarting the computer.
- USB Hubs can be the cause of connection issues. If you're having problems, try checking whether the 3DS connects fine or not without any other devices connected.
- Current font in use: OFL Sorts Mill Goudy TT

