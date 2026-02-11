# GServer-v2
[![Build Status](https://jenkins.amigadev.com/job/GServer-v2/job/master/badge/icon)](https://jenkins.amigadev.com/job/GServer-v2/job/master/)

Created by: Joey, Nalin, dufresnep, Codr, Marlon.
Based off the original work by 39ster.
For their additional work on the old gserver, special thanks go to:
	Agret, Beholder, Joey, Marlon, Nalin, and Pac.
	
## Building

### Required dependencies
- C++23 compiler (min supported: GCC 14, Clang 18, MSVC 2022 17.7)
- Java JRE
- CMake 3.28
- Ninja build system
- vcpkg (https://vcpkg.io/en/getting-started)

Linux users can install `openjdk-21-jre` or similar via their package manager, Windows users can install from [Microsoft](https://learn.microsoft.com/en-us/java/openjdk/download) or via `winget`.

vcpkg needs to be installed and the `VCPKG_ROOT` environment variable needs to be set to the location where it exists.
The directory should be writable by the user running the build (unless you want to spend extra time reading documentation and configuring the software).

### Build with CMake

If using command-line cmake, you would start a build like so:
```
cmake --preset "Windows Release" -G Ninja -B build
cmake --build build
```

On a Linux build, you would use `Linux Release` as the preset.

IDEs such as Visual Studio and CLion have CMake support built-in and can be used to easily build and configure the projects.

## Quick Start Instructions

How-to setup a server:

1. Under the accounts folder, rename the text file `YOURACCOUNT.txt` to your account name.  For example: `KuJi.txt`
2. Modify `defaultaccount.txt` to your liking.  This is the default settings new players will start with.  It can also be modified via RC.
3. Open `config/serveroptions.txt` and edit it to your liking (like selecting a server generation).  Be sure to modify the settings under "Private server options".
4. Find the line that starts with `staff=` in `config/serveroptions.txt`.  Replace `YOURACCOUNT` with your account name.  Anybody who needs RC access must be added to this line with their account names separated by commas.  Additionally, RC users must have their IP range changed to at least `*.*.*.*` in their account to connect.
5. Run `gs2emu.exe` -- enjoy.

Next steps:

[Start here](bin/docs/start-here.md) for more detailed instructions on how to set up and configure your server.

## Documentation

Documentation is available in the `/docs/` folder.
