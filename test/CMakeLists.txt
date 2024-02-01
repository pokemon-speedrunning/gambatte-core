find_package(Python3 COMPONENTS Interpreter)
find_package(PNG)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/bios.gb")
  message(STATUS "bios.gb could NOT be found")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/bios.gbc")
  message(STATUS "bios.gbc could NOT be found")
endif()

if(NOT Python3_Interpreter_FOUND OR NOT PNG_FOUND)
  message(FATAL_ERROR "Python3 and libpng are required to build tests")
endif()

file(GLOB_RECURSE GB_ASM_SOURCES "hwtests/**/*.asm")
message(STATUS "Processing assembly files...")
execute_process(
  COMMAND "${Python3_EXECUTABLE}" "qdgbas.py" ${GB_ASM_SOURCES}
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  OUTPUT_QUIET
)
message(STATUS "Done processing assembly files")
file(GLOB_RECURSE GB_ASM_BIN "hwtests/**/*.gb")

add_executable(testrunner testrunner.cpp)
target_compile_options(testrunner PRIVATE ${LIBGAMBATTE_COMPILE_FLAGS})
target_include_directories(testrunner PRIVATE ${LIBGAMBATTE_COMMON_PRIVATE_HEADERS})
target_link_libraries(testrunner PRIVATE libgambatte::libgambatte-static PNG::PNG)
set_target_properties(testrunner PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED TRUE
  CXX_EXTENSIONS OFF
)

add_test(
  NAME testrunner
  COMMAND testrunner ${GB_ASM_BIN}
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)

enable_language(C)
add_executable(testcapi testcapi.c)
target_compile_options(testcapi PRIVATE ${LIBGAMBATTE_COMPILE_FLAGS})
target_link_libraries(testcapi PRIVATE libgambatte::libgambatte-static)
set_target_properties(testcapi PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED TRUE
  C_EXTENSIONS OFF
)

add_test(
  NAME test_c_api
  COMMAND testcapi
)
