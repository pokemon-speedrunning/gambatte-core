# Setting Up the Build Environment

These instructions explain how to set up an environment for building the core libgambatte library from source on different operating systems. Once you've set up your build environment, see the main [README.md](README.md) for build instructions and information on running the test suite.

Running the hwtests suite requires installing two additional dependencies (`python3`, and `libpng`); this can be skipped if you have no intention of using the testrunner (we primarily use the test suite to confirm no regressions in accuracy were introduced in our changes).

---

Platform-specific instructions:

- [Windows](#windows)
- [macOS](#macos)
- [Linux](#linux)

---

## Windows

**NOTE:** The instructions below assume you're installing 64-bit MSYS2 and using the 32-bit MinGW toolchain (they work with other configurations, but the commands won't be exactly the same).

- Choice of 32-bit vs 64-bit for MSYS2 doesn't really matter; either is fine
- There's currently no benefit to building a 64-bit version of libgambatte on Windows, so 32-bit is used for the release binaries

**NOTE:** At the end of these steps, to do any build tasks related to libgambatte, open the MSYS2 MinGW 32-bit shell (under the MSYS2 folder in the Start menu; **\*\*must be\*\*** this specific shell in order for building to work).

### Basic steps

Do the following:

- Install [MSYS2](https://www.msys2.org/) by selecting the one-click installer exe for x86_64

- Run MSYS2 shell and update MSYS2 core components and packages _(copied from [here](https://www.msys2.org/wiki/MSYS2-installation/#iii-updating-packages))_:

```
pacman -Syuu
```

Follow the instructions. Repeat this step until it says there are no packages to update.
See the above link if you have an older installation of MSYS2 and/or pacman.

- Install MSYS2 packages for general build development environment:

```
pacman -S base-devel \
          git \
          mingw-w64-i686-toolchain \
          mingw-w64-i686-cmake \
          mingw-w64-i686-ninja \
          mingw-w64-i686-zlib
```

### Testrunner-specific steps

- Install the `libpng` and `python3` packages:

```
pacman -S mingw-w64-i686-libpng mingw-w64-i686-python3
```

### Visual Studio users

As there is no standard package manager for Visual Studio, you will need to either manually build zlib (and libpng for test), or use a package manager such as [vcpkg](https://github.com/microsoft/vcpkg).

You will also need to manually install [CMake](https://cmake.org/) and to make sure to add it to your `PATH` during installation.

## macOS

### Basic steps

_Open a terminal and do the following:_

- Install Xcode Command Line Tools and accept the license (if not already installed):

```
xcode-select --install
sudo xcodebuild -license accept
```

- Install Homebrew if not already installed _(copied from [here](https://brew.sh/))_:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

- Install the `cmake`, `ninja`, and `zlib` packages:

```
brew install cmake ninja zlib
```

### Testrunner-specific steps

- Install the `libpng` and `python3` packages:

```
brew install libpng python3
```

## Linux

### Basic steps

_Open a terminal and do the following:_

- Install build dependencies:

- Ubuntu/Debian
  ```
  sduo apt update && sudo apt upgrade -y
  sudo apt install build-essential git cmake ninja-build libz-dev
  ```
- Fedora/REHL
  ```
  sudo dnf update -y
  sudo dnf groupinstall 'Development Tools'
  sudo dnf install git cmake ninja-build zlib-devel
  ```
- Arch Linux
  ```
  sudo pacman -Syu
  sudo pacman -S base-devel git cmake ninja zlib
  ```

### Testrunner-specific steps

Install the `libpng` and `python3` packages:

- Ubuntu/Debian
  ```
  sudo apt install libpng-dev python3
  ```
- Fedora/REHL
  ```
  sudo dnf install libpng-devel python3
  ```
- Arch Linux
  ```
  sudo pacman -S libpng python3
  ```
