# Abstract
This is the repository for SALMO, the Solar Azimuth and Longitude Motorized lOcator. It is a PCB used for driving a tracking solar panel system, using GPS location and a MPPT algorithm to maximise the incident power.  
The official name of the project is `PPS2021_SUN_TRK`

# Repo structure & guidelines

## Commits

A good commit message should be preceded by a prefix, then a colon and then a brief and descriptive comment on the changes made.  
All commits should be written in the present simple tense (eg. add file, modify this, edit that).  
Commits may also include longer descriptions in the second argument of a commit message.

Following are the prefix conventions for this repository.

- `hw:` for hardware (schematic, pcb, ...)
- `fw:` for firmware
- `sw:` for software (interfaces on a pc and such)
- `docs:` for documentation
- `notes:` for lecture notes
- `chore:` for general tasks (file management, moving stuff around, ...)

:x: `git commit -m "Added some features to code"`  
:heavy_check_mark: `git commit -m "fw: add uart implementation to gps driver"`

## File naming

All files must have no spaces and should be lowercase.

:x: Name of file.ext  
:heavy_check_mark: name-of-file.ext 

# :rainbow: Building process
    sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
    git pull
    git submodule update --init --recursive

Add to your shell profile (`~/.bashrc` or `~/.zshrc`) the current directory (pico-sdk folder) as env variable:
    
`export PICO_SDK_PATH=<path_to>/pico-sdk`

and then:

    cd SALMO_pico_fw
    mkdir build
    cd build 
    cmake ..
    cd src
    make -j4

After this process some binaries files will be created. <br>The suitable one is the `.uf2` file, which is located into `SALMO_pico_fw/build/src`.<br>
Now you can start the board in bootloader mode:
1. Disconnect the board
1. Hold the `BOOTSEL` button
1. Connect the device to your computer

Then drag and drop the `.uf2` file to `RPI-RP2` mass storage device.

## Picotool
If you want to install picotool and easily flash when RP2040 is not in BOOTSEL mode, you need to follow these steps:

Linux:

    sudo apt install build-essential pkg-config libusb-1.0-0-dev
    
Mac:

    xcode-select --install
    brew install libusb
    brew link --overwrite libusb
    
and then do:

    cd picotool
    mkdir build
    cd build
    cmake ..
    make

After that, you must give execution permissions to the scripts:

    cd ../../SALMO_pico_fw/src
    sudo chmod +x build.sh
    sudo chmod +x flash.sh
    sudo chmod +x build_and_flash.sh

If something on MacOS doesn't work use

    sudo chown -R {username}:{workgroup} ppse-2021

Then, you can simply navigate to `SALMO_pico_fw/src/` and execute `build.sh`, `flash.sh` or `build_and_flash.sh` to respectively build, flash or build and flash the project. :nail_care:

# :briefcase: Adding new drivers or libraries
If you want to add a new driver or library please keep this tree structure
```
📦 SALMO_pico_fw
 ┣ 📂 build
 ┣ 📂 src
 ┣ 📂 your_lib
 ┃ ┗ 📂 your_lib docs
 ┃ ┗ 📜 CMakeLists.txt
 ┃ ┗ 📜 your_lib.c.
 ┃ ┗ 📜 your_lib.h
```
Every library needs some sort of documentation, and of course a cmake file!

# OLD :globe_with_meridians: : :boom: Compilation routine for the algo

    cd fw-dev
    cd tracking-algotithm
    cd algo

:godmode: For real optimization wizards

    gcc sun_tracker.c -O2 -lm -o out 
    ./out

:hatched_chick: For normal people

    gcc sun_tracker.c -lm  
    ./a.out

:zap: Usage with args

    ./out {year} {month} {day} {hour} {minute} {second} {latitude} {longitude}
