cmake_minimum_required(VERSION 3.12)

option(LIBGAMBATTE_BUILD_SHARED "Build the shared version of libgambatte" ON)
option(LIBGAMBATTE_BUILD_STATIC "Build the static version of libgambatte" ON)
option(LIBGAMBATTE_ENABLE_ZIP "Build zip support using zlib" ON)
option(LIBGAMBATTE_ENABLE_TESTING "Build tests" ON)

if(LIBGAMBATTE_ENABLE_ZIP)
  list(APPEND VCPKG_MANIFEST_FEATURES "zip")
endif()

if(LIBGAMBATTE_ENABLE_TESTING)
  list(APPEND VCPKG_MANIFEST_FEATURES "testing")
endif()

project(libgambatte CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(LIBGAMBATTE_IS_SUBPROJECT FALSE)
else()
  set(LIBGAMBATTE_IS_SUBPROJECT TRUE)
endif()

include(GNUInstallDirs)

set(LIBGAMBATTE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/libgambatte" CACHE STRING "Directory to install the CMake package files to")
set(LIBGAMBATTE_INSTALL_PKGCONFDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig" CACHE STRING "Directory to install the pkg-config file to")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT LIBGAMBATTE_BUILD_SHARED AND NOT LIBGAMBATTE_BUILD_STATIC)
  message(FATAL_ERROR "Both shared and static libraries have been disabled. Turn on at least one.")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  message(STATUS "No build type was specified, defaulting to: ${CMAKE_BUILD_TYPE}")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;MinSizeRel;RelWithDebInfo")
endif()

include(GetAvailableCompileFlags)
get_available_compile_flags(LIBGAMBATTE_COMPILE_FLAGS)

set(LIBGAMBATTE_SOURCES
  "libgambatte/src/bitmap_font.cpp"
  "libgambatte/src/cpu.cpp"
  "libgambatte/src/gambatte.cpp"
  "libgambatte/src/initstate.cpp"
  "libgambatte/src/interrupter.cpp"
  "libgambatte/src/interruptrequester.cpp"
  "libgambatte/src/loadres.cpp"
  "libgambatte/src/memory.cpp"
  "libgambatte/src/newstate.cpp"
  "libgambatte/src/sound.cpp"
  "libgambatte/src/state_osd_elements.cpp"
  "libgambatte/src/statesaver.cpp"
  "libgambatte/src/tima.cpp"
  "libgambatte/src/video.cpp"
  "libgambatte/src/mem/camera.cpp"
  "libgambatte/src/mem/cartridge.cpp"
  "libgambatte/src/mem/huc3_chip.cpp"
  "libgambatte/src/mem/infrared.cpp"
  "libgambatte/src/mem/memptrs.cpp"
  "libgambatte/src/mem/pakinfo.cpp"
  "libgambatte/src/mem/remote.cpp"
  "libgambatte/src/mem/rtc.cpp"
  "libgambatte/src/mem/sgb.cpp"
  "libgambatte/src/mem/time.cpp"
  "libgambatte/src/mem/mbc/huc1.cpp"
  "libgambatte/src/mem/mbc/huc3.cpp"
  "libgambatte/src/mem/mbc/m161.cpp"
  "libgambatte/src/mem/mbc/mbc0.cpp"
  "libgambatte/src/mem/mbc/mbc1.cpp"
  "libgambatte/src/mem/mbc/mbc2.cpp"
  "libgambatte/src/mem/mbc/mbc3.cpp"
  "libgambatte/src/mem/mbc/mbc5.cpp"
  "libgambatte/src/mem/mbc/mmm01.cpp"
  "libgambatte/src/mem/mbc/pocket_camera.cpp"
  "libgambatte/src/mem/mbc/wisdom_tree.cpp"
  "libgambatte/src/mem/snes_spc/dsp.cpp"
  "libgambatte/src/mem/snes_spc/SNES_SPC.cpp"
  "libgambatte/src/mem/snes_spc/SNES_SPC_misc.cpp"
  "libgambatte/src/mem/snes_spc/SNES_SPC_state.cpp"
  "libgambatte/src/mem/snes_spc/spc.cpp"
  "libgambatte/src/mem/snes_spc/SPC_DSP.cpp"
  "libgambatte/src/mem/snes_spc/SPC_Filter.cpp"
  "libgambatte/src/sound/channel1.cpp"
  "libgambatte/src/sound/channel2.cpp"
  "libgambatte/src/sound/channel3.cpp"
  "libgambatte/src/sound/channel4.cpp"
  "libgambatte/src/sound/duty_unit.cpp"
  "libgambatte/src/sound/envelope_unit.cpp"
  "libgambatte/src/sound/length_counter.cpp"
  "libgambatte/src/video/ly_counter.cpp"
  "libgambatte/src/video/lyc_irq.cpp"
  "libgambatte/src/video/next_m0_time.cpp"
  "libgambatte/src/video/ppu.cpp"
  "libgambatte/src/video/sprite_mapper.cpp"
)

file(GLOB LIBGAMBATTE_PUBLIC_HEADERS "libgambatte/include/*.h")
file(
  COPY ${LIBGAMBATTE_PUBLIC_HEADERS}
  DESTINATION "${libgambatte_BINARY_DIR}/include/gambatte"
  PATTERN "config.h" EXCLUDE
)

set(LIBGAMBATTE_COMMON_PRIVATE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/common")

set(LIBGAMBATTE_SUPPORT_ZIP FALSE)

if(LIBGAMBATTE_ENABLE_ZIP)
  find_package(ZLIB)
  set(LIBGAMBATTE_SUPPORT_ZIP ${ZLIB_FOUND})
endif()

if(LIBGAMBATTE_SUPPORT_ZIP)
  enable_language(C)
  list(APPEND LIBGAMBATTE_SOURCES
    "libgambatte/src/file/unzip/unzip.c"
    "libgambatte/src/file/unzip/ioapi.c"
    "libgambatte/src/file/file_zip.cpp"
  )
else()
  list(APPEND LIBGAMBATTE_SOURCES
    "libgambatte/src/file/file.cpp"
    "libgambatte/src/file/crc32.cpp"
  )
endif()

include(CheckIncludeFileCXX)

foreach(_header "cstdint" "stdint.h")
  string(TOUPPER "${_header}" _header_upper_name)
  string(MAKE_C_IDENTIFIER "${_header_upper_name}" _header_c_name)
  check_include_file_cxx("${_header}" "HAVE_${_header_c_name}")
endforeach()

find_package(Git)
set(LIBGAMBATTE_REVISION "-1")

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-list HEAD --count
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT_COUNT)
  string(REPLACE "\n" "" GIT_COMMIT_COUNT "${GIT_COMMIT_COUNT}")

  if(GIT_COMMIT_COUNT MATCHES "^[0-9]*$")
    set(LIBGAMBATTE_REVISION "${GIT_COMMIT_COUNT}")
  endif()
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
  "#include <stdlib.h>
   static void f(void) __attribute__ ((visibility(\"default\")));
   int main(void) {return 0;}"
  LIBGAMBATTE_HAVE_ATTRIBUTE_VISIBILITY_DEFAULT
)

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/config.h.in"
  "${libgambatte_BINARY_DIR}/include/gambatte/config.h"
)

if(LIBGAMBATTE_BUILD_SHARED)
  add_library(libgambatte-shared SHARED ${LIBGAMBATTE_SOURCES} "libgambatte/src/cinterface.cpp")
  add_library(libgambatte::libgambatte-shared ALIAS libgambatte-shared)
  list(APPEND LIBGAMBATTE_TARGETS libgambatte-shared)
endif()

if(LIBGAMBATTE_BUILD_STATIC)
  add_library(libgambatte-static STATIC ${LIBGAMBATTE_SOURCES})
  add_library(libgambatte::libgambatte-static ALIAS libgambatte-static)
  list(APPEND LIBGAMBATTE_TARGETS libgambatte-static)
endif()

# Create the same alias as the exported CMake config
if(LIBGAMBATTE_BUILD_SHARED)
  add_library(libgambatte::libgambatte ALIAS libgambatte-shared)
else()
  add_library(libgambatte::libgambatte ALIAS libgambatte-static)
endif()

set_target_properties(${LIBGAMBATTE_TARGETS} PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED TRUE
  CXX_EXTENSIONS OFF
  OUTPUT_NAME "gambatte"
)

# MSVC and non-mingw Clang toolchain use .lib for the shared implib and the static library
# Make sure the names don't clash since both are installed to lib/
if(WIN32 AND NOT MINGW AND LIBGAMBATTE_BUILD_STATIC)
  set_target_properties(libgambatte-static PROPERTIES OUTPUT_NAME "gambatte-static")
endif()

foreach(_target IN LISTS LIBGAMBATTE_TARGETS)
  target_compile_options(${_target} PRIVATE ${LIBGAMBATTE_COMPILE_FLAGS})

  target_include_directories(${_target}
    PUBLIC
    $<BUILD_INTERFACE:${libgambatte_BINARY_DIR}/include/gambatte>
    $<INSTALL_INTERFACE:include/gambatte>
    PRIVATE
    "${LIBGAMBATTE_COMMON_PRIVATE_HEADERS}"
    "libgambatte/src"
  )

  if(LIBGAMBATTE_SUPPORT_ZIP)
    target_link_libraries(${_target} PRIVATE ZLIB::ZLIB)
  endif()

  install(TARGETS ${_target}
    EXPORT "${_target}-target"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )
  install(EXPORT "${_target}-target"
    FILE "${_target}-target.cmake"
    NAMESPACE libgambatte::
    DESTINATION "${LIBGAMBATTE_INSTALL_CMAKEDIR}"
  )

  # Export the targets for consuming from the build directory
  export(EXPORT "${_target}-target"
    FILE "${libgambatte_BINARY_DIR}/${_target}-target.cmake"
    NAMESPACE libgambatte::
  )
endforeach()

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/libgambatte-config.cmake.in"
  "${libgambatte_BINARY_DIR}/install-config/libgambatte-config.cmake"
  INSTALL_DESTINATION "${LIBGAMBATTE_INSTALL_CMAKEDIR}"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "libgambatte-config-version.cmake"
  VERSION "${LIBGAMBATTE_REVISION}"
  COMPATIBILITY ExactVersion
)

install(FILES
  "${libgambatte_BINARY_DIR}/install-config/libgambatte-config.cmake"
  "${libgambatte_BINARY_DIR}/libgambatte-config-version.cmake"
  DESTINATION "${LIBGAMBATTE_INSTALL_CMAKEDIR}"
)

install(DIRECTORY "${libgambatte_BINARY_DIR}/include/gambatte"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Write a special config for consuming from the build directory
set(PACKAGE_INIT "get_filename_component(PACKAGE_PREFIX \"\${CMAKE_CURRENT_LIST_DIR}\" ABSOLUTE)")
configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/libgambatte-config.cmake.in"
  "${libgambatte_BINARY_DIR}/libgambatte-config.cmake"
  @ONLY
)
unset(PACKAGE_INIT)

# Create and install a pkg-config file
set(LIBGAMBATTE_PC_REQUIRES)
set(LIBGAMBATTE_PC_REQUIRES_PRIVATE)

if(LIBGAMBATTE_SUPPORT_ZIP)
  # Make it a public dependency if only static is built
  if(NOT LIBGAMBATTE_BUILD_SHARED)
    # For macOS, the zlib pc file is provided by Homebrew
    list(APPEND LIBGAMBATTE_PC_REQUIRES "zlib")
  else()
    list(APPEND LIBGAMBATTE_PC_REQUIRES_PRIVATE "zlib")
  endif()
endif()

# Ensure the paths written in the pkg-config are relative to the prefix
if(IS_ABSOLUTE "${LIBGAMBATTE_INSTALL_PKGCONFDIR}")
  file(RELATIVE_PATH
    LIBGAMBATTE_PC_TO_PREFIX
    "${LIBGAMBATTE_INSTALL_PKGCONFDIR}"
    "${CMAKE_INSTALL_PREFIX}"
  )
else()
  file(RELATIVE_PATH
    LIBGAMBATTE_PC_TO_PREFIX
    "${CMAKE_INSTALL_PREFIX}/${LIBGAMBATTE_INSTALL_PKGCONFDIR}"
    "${CMAKE_INSTALL_PREFIX}"
  )
endif()

if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
  file(RELATIVE_PATH
    LIBGAMBATTE_PC_INCLUDEDIR
    "${CMAKE_INSTALL_PREFIX}"
    "${CMAKE_INSTALL_INCLUDEDIR}"
  )
else()
  set(LIBGAMBATTE_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  file(RELATIVE_PATH
    LIBGAMBATTE_PC_LIBDIR
    "${CMAKE_INSTALL_PREFIX}"
    "${CMAKE_INSTALL_LIBDIR}"
  )
else()
  set(LIBGAMBATTE_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
endif()

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/libgambatte.pc.in"
  "${libgambatte_BINARY_DIR}/libgambatte.pc"
  @ONLY
)

install(FILES "${libgambatte_BINARY_DIR}/libgambatte.pc"
  DESTINATION "${LIBGAMBATTE_INSTALL_PKGCONFDIR}"
)

if(LIBGAMBATTE_ENABLE_TESTING)
  if(NOT LIBGAMBATTE_BUILD_STATIC)
    message(FATAL_ERROR "Building the static library is required to run tests")
  endif()

  enable_testing()
  add_subdirectory(test)
endif()
