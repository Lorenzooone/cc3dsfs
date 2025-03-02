# cc3dsfs

cc3dsfs is a multi-platform capture and display program for [3dscapture's](https://3dscapture.com/) N3DSXL, 3DS and DS capture boards written in C++.
The main goal is to offer the ability to use the Capture Card with a TV, via fullscreen mode.

IS Nitro Emulator (newer revisions), IS Nitro Capture and IS TWL Capture support is also present. Though results may vary (and the amount of video delay may be significantly higher based on the cable used).

Support for a really old Optimize board is also present, used with Nisetro DS capture boards.

## Features

- Performance-focused design, with low latency for both audio and video (measured to oscillate between 1 and 2 frames on 120hz displays).
- Option to split the screens in separate windows, to address them separately.
- Make your game run in fullscreen mode. If you own multiple displays, you can even use one per-window.
- Many builtin crop options for the screens.
- Many other settings, explained in [Controls](#Controls).
- Prebuilt executables available in the [Releases](https://github.com/Lorenzooone/cc3dsfs/releases/latest) page for ease of use.
- Various command line arguments to personalize how the software should behave. Use -h or --help as an argument to display the help message.

_Note: On 3DS, DS, GBA, GBC and GB games boot in scaled resolution mode by default. Holding START or SELECT while launching these games will boot in native resolution mode._

_Note: Make sure the 3DS audio is not set to Surround._

## Dependencies

cc3dsfs has three build dependencies: CMake, g++ and git.
Make sure all are installed.
On MacOS, [Homebrew](https://brew.sh/) can be used to install both CMake and git. An automatic popup should appear to install g++ at Compile time.

cc3dsfs has five library dependencies: [FTDI's D3XX driver](https://ftdichip.com/drivers/d3xx-drivers/), [FTDI's D2XX driver](https://ftdichip.com/drivers/d2xx-drivers/) (on Windows), [libusb](https://libusb.info/), [libftdi](https://www.intra2net.com/en/developer/libftdi/) and [SFML](https://www.sfml-dev.org/).
All of them should get downloaded automatically via CMake during the building process.

Linux users who wish to compile this, will also need to install the SFML dependencies. Different distributions will require slightly different processes.
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
    libflac-dev \
    libvorbis-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libdrm-dev \
    libgbm-dev \
    libfreetype-dev \
    xorg-dev
```

Additionally, when compiling for a Raspberry Pi, install gpiod and libgpiod-dev.

On Windows, you may need to install the Visual C++ Redistributable set of libraries. They are available here: [Official Microsoft VC Redist Link](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).

## Compilation

To compile the program, assuming CMake, git and g++ are installed on the system, this is the command which should be launched:

```
cmake -B build ; cmake --build build --config Release
```

This will download FTD3XX, FTD2XX, libusb and SFML, which may take a while during the first execution of the command. Later runs should be much faster.
On MacOS, you may also be prompted to install the Apple Command Line Developer Tools first.

When compiling on a Raspberry Pi, to enable usage of GPIO, use:
```
cmake -B build -DRASPBERRY_PI_COMPILATION=TRUE ; cmake --build build --config Release
```

### Docker Compilation

Alternatively, one may use Docker to compile the Linux version for its different architectures by running: `docker run --rm -it -v ${PWD}:/home/builder/cc3dsfs lorenzooone/cc3dsfs:<builder>`
The following builders are available: builder32, builder64, builderarm32 and builderarm64.

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
- __C key__: Cycles to the next Crop mode for the focused window. For 3DS, the currently supported cropping modes are: 3DS, 16:10 DS, scaled DS, native DS, scaled GBA, native GBA, scaled GB, native GB, scaled SNES, native SNES and native NES. For DS, the currently supported cropping modes are: DS, Top GBA and Bottom GBA. There are also game-specific cropping modes which can be enabled via the Video Settings.
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
- __,/. key__: Decrements/Increments the volume by 5 units. 0 is the minimum. 200 is the maximum.
- __N key__: Restarts the audio output. Useful if the audio stops working after changing OS settings.
- __F1 - F4 keys__: Loads from layouts 1 through 4 respectively. To access more profiles, the GUI must be used.
- __F5 - F8 keys__: Saves to layouts 1 through 4 respectively. To access more profiles, the GUI must be used.
- __Esc key__: Properly quits the program.

_Note: The volume is independent of the actual volume level set with the physical slider on the console._

## Profiles/Layouts

When starting the program for the first time, a message indicating a load failure for the cc3dsfs.cfg file will be displayed, and the same will occur when attempting to load from any given layout file if it hasn't been saved to before. That is normal. These files must be created by the program first before they can be loaded from. The program saves its current configuration to the cc3dsfs.cfg file when the program is successfully closed, creating the file if it doesn't already exist, and loading from it every time at startup.

The current configuration can be saved to various extra profiles, creating the given file if it doesn't already exist. Changing settings after a layout is loaded will not automatically overwrite its file. To make the changes permanent, saving the profile again is required.

The name of profiles can be changed by altering the __name__ field in its file.

On Linux and MacOS, the profiles can be found at the "${HOME}/.config/cc3dsfs" folder. By default, "/home/<user_name>/.config/cc3dsfs".

On Windows, the profiles can be found in the ".config/cc3dsfs" folder inside the directory in which the program runs from.

## Notes
- On Linux, you may need to include the udev USB access rules. You can use the .rules files available in the repository's usb\_rules directory, or define your own. For ease of use, releases come bundled with a script to do it named install\_usb\_rules.sh. It may require elevated permissions to execute properly. You may get a permission error if the rules are not installed.
- At startup, the audio may be unstable. It should fix itself, if you give it enough time.
- If, at first, the connection to the 3DS/DS fails, reconnect the 3DS/DS and then try again. If that also doesn't work, try restarting the program. If that also doesn't work, try restarting the computer.
- USB Hubs can be the cause of connection issues. If you're having problems, try checking whether the 3DS/DS connects fine or not without any other devices connected.
- Current font in use: OFL Sorts Mill Goudy TT
- Enabling Slow Poll may slightly boost the FPS of the software, at the cost of an extremely slight decrease in frame latency, and slower reaction times of the software to key presses. Disabled by default (as when the FPS are greater than the CC's, it's not reccomended).
- On MacOS, you may get a notice about Apple being unable to check for malware, or the developer being unknown. To open the program regardless of that, check the [Official Guide for "Opening Applications from Unknown Developers" from Apple, for your version of MacOS](https://support.apple.com/guide/mac-help/open-a-mac-app-from-an-unknown-developer-mh40616/mac).
- Certain TVs/Monitors may add some audio delay for the purpose of video/lip syncing. If you're experiencing too much audio delay when using this software, try checking in the TV/Monitor settings whether you can reduce that added delay. One of the names used for that setting is "Lip Sync", or something along that line.
- When using IS Nitro Emulator, IS Nitro Capture or IS TWL Capture devices on Windows, cc3dsfs is compatible with both the official driver and WinUSB, with the latter being useful if you don't have access to the official driver. To install and use WinUSB, plug in your device, download a software to install drivers like [Zadig](https://zadig.akeo.ie/), select the device in question and select WinUSB. Then install the driver and wait for it to finish. The devices should now work properly with this application.
- When using the new 2024 Loopy DS Capture Card on Windows, the default driver (FTD2XX) adds one extra frame of latency. To remove that, consider switching to WinUSB as the driver. To change driver, download a software to install drivers like [Zadig](https://zadig.akeo.ie/), select the device in question and select WinUSB. Then install the driver and wait for it to finish. The application will now use WinUSB, with better latency (the serial shown in the Status menu will have an l where there previously was a d).
