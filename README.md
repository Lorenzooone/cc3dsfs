# cc3dsfs

cc3dsfs is a multi-platform capture and display program for [3dscapture's](https://3dscapture.com/) N3DSXL, 3DS and DS (old) capture boards written in C++.
The main goal is to offer the ability to use the Capture Card with a TV, via fullscreen mode.

## Features

- Performance-focused design, with low latency for both audio and video (measured to oscillate between 1 and 2 frames).
- Option to split the screens in separate windows, to address them separately.
- Make your game run in fullscreen mode. If you own multiple displays, you can even use one per-window.
- Many builtin crop options for the screens.
- Many other settings, explained in [Controls](#Controls).

_Note: On 3DS, DS, GBA, GBC and GB games boot in scaled resolution mode by default. Holding START or SELECT while launching these games will boot in native resolution mode._

_Note: Make sure the 3DS audio is not set to Surround._

## Dependencies

cc3dsfs has three build dependencies: CMake, g++ and git.
Make sure all are installed.
On MacOS, [Homebrew](https://brew.sh/) can be used to install both CMake and git. An automatic popup should appear to install g++ at Compile time.

cc3dsfs has three library dependencies: [FTDI's D3XX driver](https://ftdichip.com/drivers/d3xx-drivers/), [libusb](https://libusb.info/) and [SFML](https://www.sfml-dev.org/).
All of them should get downloaded automatically via CMake during the building process.

Linux users will also need to install the SFML dependencies. Different distributions will require slightly different processes.
Below, the command for Debian-based distributions, which also lists the required libraries.

```
sudo apt update
sudo apt install \
    g++ \
    git \
    cmake \
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

Additionally, when compiling for a Raspberry Pi, install gpiod and libgpiod-dev.

## Compilation

To compile the program, assuming CMake, git and g++ are installed on the system, this is the command which should be launched:

```
cmake -B build ; cmake --build build --config Release
```

This will download FTD3XX, libusb and SFML, which may take a while during the first execution of the command. Later runs should be much faster.
On MacOS, you may also be prompted to install the Apple Command Line Developer Tools first.

## Controls

The software has a GUI which exposes all of the available settings. There are also various available keyboard shortcuts which allow quicker access to the options.
Most of the settings are explained in [Keyboard shortcuts](#Keyboard-shortcuts).

### Keyboard controls
- __Enter key__: Used to open the GUI when it is not shown, as well as to confirm the selection of an option. Holding it for 30 seconds will reset the application to its defaults.
- __Arrow keys__: Used to change the selected option in the GUI.

### Mouse controls
- __Right click__: Used to open the GUI when it is not shown. Holding it for 30 seconds will reset the application to its defaults.
- __Left click__: Used to confirm the selection of an option.

### Joystick controls
- __Option/Share buttons__: Used to open the GUI when it is not shown.
- __A/B/X/Y buttons__: Used to confirm the selection of an option. Holding B (X on a PS5 controller) for 30 seconds will reset the application to its defaults.
- __Dpad/Sitcks__: Used to change the selected option in the GUI.

_Note: Currently only tested using a PS5 controller._

### Keyboard shortcuts
- __O key__: Open/Close connection to the 3DS/DS.
- __F key__: Toggles Fullscreen mode on/off. Only guaranteed to work on the primary monitor. When using certain setups, it also works for multiple monitors.
- __S key__: Swaps between Split mode and Joint mode. In Split mode, each screen has its own window. In Joint mode, they are combined into a single one.
- __C key__: Cycles to the next Crop mode for the focused window. For 3DS, the currently supported cropping modes are: 3DS, 16:10 DS, scaled DS, native DS, scaled GBA, native GBA, scaled GB, native GB, scaled SNES, native SNES and native NES. For DS, the currently supported cropping modes are: DS, Top GBA and Bottom GBA.
- __-/0 key__: Decrements/Increments the scaling by 0.5x for the non-Fullscreen focused window. 1.0x is the minimum. 45.0x is the maximum.
- __Y/U key__: In Joint Fullscreen mode, increases the size of the top/bottom screen. Corresponds to the _Screens ratio_ Video Setting.
- __Z/X key__: Decreases/Increases the Menu Scaling multiplier by 0.1. 0.3 is the minimum. 10.0 is the maximum.
- __T key__: Moves the relative position of the bottom screen inside of the joint mode window clockwise. Corresponds to the _Screens Relative Positions_ Video Setting.
- __V key__: Toggles VSync on/off for the focused window. VSync prevents screen tearing. However, at low refresh rates (60 hz), it may significantly increase image delay.
- __A key__: Toggles Async on/off for the focused window. Having it off ensures all the available windows show the same frame. If you're having framerate issues, turning this setting on/off could help.
- __B key__: Toggles Blur on/off for the focused window. This is only noticeable at scales of 1.5x or greater.
- __6 key__: In Joint mode, changes the position of the smaller screen relative to the bigger one between 0, 1/2 Max and Max. Accessible via either the _Offset Settings_, or the _Screen Offset_ Video Settings.
- __7 key__: In Joint Fullscreen mode, changes the distance from the upper screen between 0, 1/2 Max and Max. Accessible via the _Offset Settings_ Video Setting.
- __4 key__: In Fullscreen mode, changes the X distance from the border between 0, 1/2 Max and Max. Accessible via the _Offset Settings_ Video Setting.
- __5 key__: In Fullscreen mode, changes the Y distance from the border between 0, 1/2 Max and Max. Accessible via the _Offset Settings_ Video Setting.
- __8/9 key__: Rotates the screen(s) of the focused window 90 degrees counterclockwise/clockwise. Accessible via the _Rotation Settings_ Video Setting.
- __H/J key__: Rotates the top screen of the focused window 90 degrees counterclockwise/clockwise. Accessible via either the _Rotation Settings_, or the _Screen Rotation_ Video Settings.
- __K/L key__: Rotates the bottom screen of the focused window 90 degrees counterclockwise/clockwise. Accessible via either the _Rotation Settings_, or the _Screen Rotation_ Video Settings.
- __R key__: Toggles extra padding on/off. It can be used to account for windows with rounded corners.
- __2/3 key__: Cycles through the available Pixel Aspect Ratios (PAR) for the top/bottom screen. Useful for VC games and for Chrono Trigger DS.
- __M key__: Toggles Audio mute on/off.
- __,/. key__: Decrements/Increments the volume by 5 units. 0 is the minimum. 100 is the maximum.
- __N key__: Restarts the audio output. Useful if the audio stops working after changing OS settings.
- __F1 - F4 keys__: Loads from layouts 1 through 4 respectively. To access more profiles, the GUI must be used.
- __F5 - F8 keys__: Saves to layouts 1 through 4 respectively. To access more profiles, the GUI must be used.
- __Esc key__: Properly quits the program.

_Note: The volume is independent of the actual volume level set with the physical slider on the console._

## Profiles/Layouts

When starting the program for the first time, a message indicating a load failure for the cc3dsfs.cfg file will be displayed, and the same will occur when attempting to load from any given layout file if it hasn't been saved to before. These files must be created by the program first before they can be loaded from. The program saves its current configuration to the cc3dsfs.cfg file when the program is successfully closed, creating the file if it doesn't already exist, and loads from it every time at startup.

The current configuration can be saved to various extra profiles, creating the given file if it doesn't already exist. Changing the configuration after a layout is loaded will not overwrite it unless the respective it is saved.

The name of profiles can be changed by altering the __name__ field in its file.

## Notes
- On Linux, you may need to include the udev USB access rules. You can use the .rules files available in the repository, or define your own.
- At startup, the audio may be unstable. It should fix itself, if you give it enough time.
- If, at first, the connection to the 3DS/DS fails, reconnect the 3DS/DS and then try again. If that also doesn't work, try restarting the program. If that also doesn't work, try restarting the computer.
- USB Hubs can be the cause of connection issues. If you're having problems, try checking whether the 3DS/DS connects fine or not without any other devices connected.
- Fullscreen mode on MacOS may mistake the screen for being bigger than what it really is. Changing the resolution to the proper one of the screen in the _Resolution Settings_ under Video Settings will fix the issue.
- Current font in use: OFL Sorts Mill Goudy TT
- Enabling Slow Poll may slightly boost the FPS of the software, at the cost of an extremely slight decrease in frame latency, and slower reaction times of the software to key presses. Disabled by default (as when the FPS are greater than the CC's, it's not reccomended).
- When compiling on a Raspberry Pi, to enable usage of GPIO, use:
```
cmake -B build -DRASPBERRY_PI_COMPILATION=TRUE ; cmake --build build --config Release
```
- On MacOS, you may get a notice about the app being damaged. It's Apple quaranteening the app. To make it work, open a terminal and run the following:
```
xattr -c ./cc3dsfs.app
```

