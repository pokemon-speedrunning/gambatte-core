# Gambatte Core

Fork of [Gambatte](https://github.com/sinamas/gambatte) (authored by sinamas), with local changes for Pokémon Speedruns, as well as other speedrunning communities. Under GPLv2.

This is the core library, for the Qt frontend, check out [Gambatte-Speedrun](https://github.com/pokemon-speedrunning/gambatte-speedrun).

---

## Building from source

The library can be built on Windows using [MSYS2](https://www.msys2.org/), on macOS using [Homebrew](https://brew.sh/), and on Linux using your system's package manager. See [INSTALL.md](INSTALL.md) for detailed information on how to set up the build environment.

The amount of setup you need to do depends on what parts of the project you are planning to use.

Open a terminal and do the following:

- Clone the project
  ```
  git clone https://github.com/pokemon-speedrunning/gambatte-core.git
  cd gambatte-core
  ```
- Configure with CMake
  ```
  cmake -S . -B build
  ```
  Here you can provide additional options such as:
  - `-G Ninja` to use ninja over the default generator
  - `-DCMAKE_BUILD_TYPE=<config>` to select between `Debug`, `Release`, etc. (default: `Release`)
  - `-DCMAKE_INSTALL_PREFIX=/path/to/install` to select where the libraries should be installed
  - `-DLIBGAMBATTE_SHARED=ON/OFF` to enable/disable building the shared library (default: `ON`)
  - `-DLIBGAMBATTE_STATIC=ON/OFF` to enable/disable building the static library (default: `ON`)
  - `-DLIBGAMBATTE_ENABLE_ZIP=ON/OFF` to enable/disable zip support (default: `ON`)
  - `-DLIBGAMBATTE_ENABLE_TESTING=ON/OFF` to enable/disable the test suite (default: `ON`)
- Build the library
  ```
  cmake --build build
  ```
- Install the library
  ```
  cmake --install build
  ```

Note: When using a multi-config generator (`Visual Studio ...`, `Ninja Multi-Config`), you must specify `--config <config>` for the `--build` and `--install` steps.

### Testrunner

To be able to run the upstream hwtests suite, you must acquire the DMG and CGB bootroms. Name the DMG bootrom `bios.gb`, the CGB bootrom `bios.gbc`, and move both into the `test` directory.

To run the test suite, you must configure the project with `LIBGAMBATTE_ENABLE_TESTING` set to `ON` and build it. You can then run the following command:

```
ctest --test-dir build
```

## Consuming the library

You can consume the library either through CMake's `find_package` or pkg-config.

### CMake package

To find and link the library, add the following to your CMakeLists:

```
find_package(libgambatte CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE libgambatte::libgambatte)
```

The package provides the following targets:

- `libgambatte::libgambatte`
- `libgambatte::libgambatte-shared`
- `libgambatte::libgambatte-static`

The `libgambatte::libgambatte` target is an alias of the shared library, if the shared library was not built the target instead aliases the static library.

It is not necessary to install the library to use it through CMake, you can set `libgambatte_DIR` to the build directory when configuring your own project:

```
cd my_project
cmake -S . -B build -Dlibgambatte_DIR=/path/to/gambatte-core/build
```

### pkg-config

For non-CMake projects, a pkg-config file is provided.

By default, it is installed to `<prefix>/lib/pkgconfig/libgambatte.pc`, this can be controlled with the `LIBGAMBATTE_INSTALL_PKGCONFDIR` option.

You can then read from it using pkg-config, make sure to set your search patch accordingly:

```
export PKG_CONFIG_PATH="/path/to/libgambatte/lib/pkgconfig:$PKG_CONFIG_PATH"
pkg-config --cflags --libs libgambatte
```
