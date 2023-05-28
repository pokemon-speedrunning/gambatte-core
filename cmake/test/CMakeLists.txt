cmake_minimum_required(VERSION 3.0)
project(libgambatte-consumer-test LANGUAGES CXX)

add_executable(example main.cpp)

find_package(libgambatte CONFIG REQUIRED)
target_link_libraries(example PRIVATE libgambatte::libgambatte-static)
set_target_properties(example PROPERTIES CXX_STANDARD 11)
