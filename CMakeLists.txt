cmake_minimum_required(VERSION 3.20)

project(machine-lab VERSION 0.2.0 LANGUAGES C CXX)

option(MACHINE_LAB_WITH_SDL "Build the SDL3 desktop display/input backend" OFF)
if(DEFINED LCOM_WITH_SDL)
  set(MACHINE_LAB_WITH_SDL "${LCOM_WITH_SDL}" CACHE BOOL
    "Build the SDL3 desktop display/input backend" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
find_package(CLI11 CONFIG QUIET)
if(NOT CLI11_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    CLI11
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.5.0.tar.gz
  )
  FetchContent_MakeAvailable(CLI11)
endif()

add_library(machinelab_core STATIC
  runtime/core/Trace.cpp
  runtime/core/IrqController.cpp
  runtime/core/Bus.cpp
  runtime/core/Machine.cpp
  runtime/core/Script.cpp
  runtime/backends/AudioBackend.cpp
  runtime/devices/Ac97.cpp
  runtime/devices/Pit8254.cpp
  runtime/devices/I8042.cpp
  runtime/devices/RtcCmos.cpp
  runtime/devices/Vbe.cpp
  runtime/devices/Uart16550.cpp
)
target_include_directories(machinelab_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)
target_compile_options(machinelab_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(machinelab_c STATIC sdk/client/src/lcom.c)
set_target_properties(machinelab_c PROPERTIES OUTPUT_NAME machinelab)
target_include_directories(machinelab_c PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
  ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_compile_options(machinelab_c PRIVATE -Wall -Wextra -Wpedantic)
add_custom_command(TARGET machinelab_c POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:machinelab_c>
    $<TARGET_FILE_DIR:machinelab_c>/${CMAKE_STATIC_LIBRARY_PREFIX}lowlab${CMAKE_STATIC_LIBRARY_SUFFIX}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:machinelab_c>
    $<TARGET_FILE_DIR:machinelab_c>/${CMAKE_STATIC_LIBRARY_PREFIX}lcom${CMAKE_STATIC_LIBRARY_SUFFIX}
)

set(MACHINE_LAB_RUNTIME_SOURCES
  runtime/main.cpp
  runtime/cli/FileOps.cpp
  runtime/cli/LabCatalog.cpp
  runtime/core/RuntimeServer.cpp
  runtime/core/PairRuntimeServer.cpp
)

if(MACHINE_LAB_WITH_SDL)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
  pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)
  list(APPEND MACHINE_LAB_RUNTIME_SOURCES runtime/backends/SdlBackend.cpp)
  list(APPEND MACHINE_LAB_RUNTIME_SOURCES runtime/backends/SdlAudioBackend.cpp)
endif()

add_executable(machinelab ${MACHINE_LAB_RUNTIME_SOURCES})
target_link_libraries(machinelab PRIVATE machinelab_core)
target_compile_options(machinelab PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(machinelab PRIVATE CLI11::CLI11)
if(MACHINE_LAB_WITH_SDL)
  target_compile_definitions(machinelab PRIVATE
    MACHINE_LAB_WITH_SDL
    MACHINE_LAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    MACHINE_LAB_BINARY_DIR="${CMAKE_BINARY_DIR}"
  )
  target_link_libraries(machinelab PRIVATE PkgConfig::SDL3 PkgConfig::SDL3_TTF)
else()
  target_compile_definitions(machinelab PRIVATE
    MACHINE_LAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    MACHINE_LAB_BINARY_DIR="${CMAKE_BINARY_DIR}"
  )
endif()

add_custom_command(TARGET machinelab POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:machinelab>
    $<TARGET_FILE_DIR:machinelab>/lowlab${CMAKE_EXECUTABLE_SUFFIX}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:machinelab>
    $<TARGET_FILE_DIR:machinelab>/lcom${CMAKE_EXECUTABLE_SUFFIX}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:machinelab>
    $<TARGET_FILE_DIR:machinelab>/lcom-run${CMAKE_EXECUTABLE_SUFFIX}
)

set(MACHINE_LAB_EXAMPLES
  hello
  audio_tone
  breakout_demo
  timer_int
  keyboard_scan
  music_maker
  mouse_packet
  rtc_date
  sdl_demo
  uart_loopback
  uart_pair
  uart_peer_sender
  uart_peer_receiver
  vbe_rectangle
  word_processor
)

foreach(example IN LISTS MACHINE_LAB_EXAMPLES)
  add_executable(${example} examples/${example}.c)
  target_link_libraries(${example} PRIVATE machinelab_c)
  target_compile_options(${example} PRIVATE -Wall -Wextra -Wpedantic)
  set_target_properties(${example} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
endforeach()

add_executable(flappy_bird
  examples/flappy/main.c
  examples/flappy/app.c
  examples/flappy/menu.c
  examples/flappy/font.c
  examples/flappy/renderer.c
  examples/flappy/sound.c
)
target_link_libraries(flappy_bird PRIVATE machinelab_c)
target_compile_options(flappy_bird PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(flappy_bird PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

file(GLOB NINJIX_SOURCES CONFIGURE_DEPENDS
  examples/ninjix/*.c
)
add_executable(ninjix ${NINJIX_SOURCES})
target_include_directories(ninjix PRIVATE examples/ninjix)
target_link_libraries(ninjix PRIVATE machinelab_c)
target_compile_options(ninjix PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(ninjix PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

file(GLOB_RECURSE PVZ_SOURCES CONFIGURE_DEPENDS
  examples/pvz/src/*.c
)
add_executable(pvz ${PVZ_SOURCES})
target_include_directories(pvz PRIVATE examples/pvz/include)
target_link_libraries(pvz PRIVATE machinelab_c)
target_compile_options(pvz PRIVATE -Wall -Wextra -Wpedantic -Wno-strict-prototypes)
set_target_properties(pvz PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

add_executable(unit_tests
  tests/unit_tests.cpp
)
target_link_libraries(unit_tests PRIVATE machinelab_core)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(unit_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

function(add_lab_solution_test lab)
  add_executable(${lab}_solution_test
    dev/lab-tests/${lab}_test.c
    ${ARGN}
  )
  target_include_directories(${lab}_solution_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dev/lab-tests
    ${CMAKE_CURRENT_SOURCE_DIR}/dev/lab-solutions/${lab}/include
  )
  target_link_libraries(${lab}_solution_test PRIVATE machinelab_c)
  target_compile_options(${lab}_solution_test PRIVATE -Wall -Wextra -Wpedantic)
  set_target_properties(${lab}_solution_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  )
endfunction()

add_lab_solution_test(lab1
  dev/lab-solutions/lab1/src/bitwise.c
  dev/lab-solutions/lab1/src/rtc_lib.c
)
add_lab_solution_test(lab2
  dev/lab-solutions/lab2/src/timer_lib.c
)
add_lab_solution_test(lab3
  dev/lab-solutions/lab3/src/keyboard_lib.c
)
add_lab_solution_test(lab4
  dev/lab-solutions/lab4/src/mouse_lib.c
)
add_lab_solution_test(lab5
  dev/lab-solutions/lab5/src/video_lib.c
)
add_lab_solution_test(lab6
  dev/lab-solutions/lab6/src/audio_lib.c
)
add_lab_solution_test(lab7
  dev/lab-solutions/lab7/src/uart_lib.c
)

if(BUILD_TESTING)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test-output)

  add_test(NAME unit COMMAND unit_tests)
  add_test(NAME lab1_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless --rtc 2026-06-16T12:34:56 -- $<TARGET_FILE:lab1_solution_test>
  )
  add_test(NAME lab2_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless -- $<TARGET_FILE:lab2_solution_test>
  )
  add_test(NAME lab3_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless --script ${CMAKE_CURRENT_SOURCE_DIR}/scripts/type_a_esc.mlabscript -- $<TARGET_FILE:lab3_solution_test>
  )
  add_test(NAME lab4_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless --script ${CMAKE_CURRENT_SOURCE_DIR}/scripts/mouse_move.mlabscript -- $<TARGET_FILE:lab4_solution_test>
  )
  add_test(NAME lab5_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless --dump-frame ${CMAKE_BINARY_DIR}/test-output/lab5-solution.ppm -- $<TARGET_FILE:lab5_solution_test>
  )
  add_test(NAME lab6_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless --audio-wav ${CMAKE_BINARY_DIR}/test-output/lab6-solution.wav -- $<TARGET_FILE:lab6_solution_test>
  )
  add_test(NAME lab7_solution
    COMMAND $<TARGET_FILE:machinelab> run --headless -- $<TARGET_FILE:lab7_solution_test>
  )
  add_test(NAME integration
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.sh
      $<TARGET_FILE_DIR:machinelab>
      ${CMAKE_BINARY_DIR}/examples
      ${CMAKE_BINARY_DIR}/test-output
  )
  set_tests_properties(integration PROPERTIES
    DEPENDS unit
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 300
  )
endif()
