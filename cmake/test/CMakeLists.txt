cmake_minimum_required(VERSION 3.6)
project(libgambatte-consumer-test LANGUAGES CXX)

option(USE_PKG_CONFIG "Use pkg-config instead of find_package" OFF)

add_executable(example main.cpp)
set_target_properties(example PROPERTIES CXX_STANDARD 11)

if(USE_PKG_CONFIG)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libgambatte REQUIRED IMPORTED_TARGET libgambatte)
  set(LIBGAMBATTE_TARGET PkgConfig::libgambatte)
else()
  find_package(libgambatte CONFIG REQUIRED)
  set(LIBGAMBATTE_TARGET libgambatte::libgambatte-static)
endif()

target_link_libraries(example PRIVATE ${LIBGAMBATTE_TARGET})
