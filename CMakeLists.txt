cmake_minimum_required(VERSION 3.20)

project(lcom-ng VERSION 0.1.0 LANGUAGES C CXX)

option(LCOM_WITH_SDL "Build the SDL3 desktop display/input backend" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
find_package(CLI11 CONFIG REQUIRED)

add_library(lcom_core STATIC
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
target_include_directories(lcom_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/common
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)
target_compile_options(lcom_core PRIVATE -Wall -Wextra -Wpedantic)

add_library(lcom-ng STATIC liblcom/src/lcom.c)
target_include_directories(lcom-ng PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/common
)
target_compile_options(lcom-ng PRIVATE -Wall -Wextra -Wpedantic)

set(LCOM_RUNTIME_SOURCES
  runtime/main.cpp
  runtime/core/RuntimeServer.cpp
)

if(LCOM_WITH_SDL)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
  pkg_check_modules(SDL3_TTF REQUIRED IMPORTED_TARGET sdl3-ttf)
  list(APPEND LCOM_RUNTIME_SOURCES runtime/backends/SdlBackend.cpp)
  list(APPEND LCOM_RUNTIME_SOURCES runtime/backends/SdlAudioBackend.cpp)
endif()

add_executable(lcom ${LCOM_RUNTIME_SOURCES})
target_link_libraries(lcom PRIVATE lcom_core)
target_compile_options(lcom PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(lcom PRIVATE CLI11::CLI11)
if(LCOM_WITH_SDL)
  target_compile_definitions(lcom PRIVATE
    LCOM_WITH_SDL
    LCOM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LCOM_BINARY_DIR="${CMAKE_BINARY_DIR}"
  )
  target_link_libraries(lcom PRIVATE PkgConfig::SDL3 PkgConfig::SDL3_TTF)
else()
  target_compile_definitions(lcom PRIVATE
    LCOM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LCOM_BINARY_DIR="${CMAKE_BINARY_DIR}"
  )
endif()

add_custom_command(TARGET lcom POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink lcom $<TARGET_FILE_DIR:lcom>/lcom-run
)

set(LCOM_EXAMPLES
  hello
  audio_tone
  timer_int
  keyboard_scan
  mouse_packet
  rtc_date
  sdl_demo
  uart_loopback
  uart_pair
  vbe_rectangle
)

foreach(example IN LISTS LCOM_EXAMPLES)
  add_executable(${example} examples/${example}.c)
  target_link_libraries(${example} PRIVATE lcom-ng)
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
target_link_libraries(flappy_bird PRIVATE lcom-ng)
target_compile_options(flappy_bird PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(flappy_bird PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

add_executable(unit_tests
  tests/unit_tests.cpp
)
target_link_libraries(unit_tests PRIVATE lcom_core)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(unit_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

function(add_lab_solution_test lab)
  add_executable(${lab}_solution_test
    tests/labs/${lab}_test.c
    ${ARGN}
  )
  target_include_directories(${lab}_solution_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/labs
    ${CMAKE_CURRENT_SOURCE_DIR}/labs/solutions/${lab}/include
  )
  target_link_libraries(${lab}_solution_test PRIVATE lcom-ng)
  target_compile_options(${lab}_solution_test PRIVATE -Wall -Wextra -Wpedantic)
  set_target_properties(${lab}_solution_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  )
endfunction()

add_lab_solution_test(lab1
  labs/solutions/lab1/src/bitwise.c
  labs/solutions/lab1/src/rtc_lib.c
)
add_lab_solution_test(lab2
  labs/solutions/lab2/src/timer_lib.c
)
add_lab_solution_test(lab3
  labs/solutions/lab3/src/keyboard_lib.c
)
add_lab_solution_test(lab4
  labs/solutions/lab4/src/mouse_lib.c
)
add_lab_solution_test(lab5
  labs/solutions/lab5/src/video_lib.c
)
add_lab_solution_test(lab6
  labs/solutions/lab6/src/audio_lib.c
)
add_lab_solution_test(lab7
  labs/solutions/lab7/src/uart_lib.c
)

if(BUILD_TESTING)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test-output)

  add_test(NAME unit COMMAND unit_tests)
  add_test(NAME lab1_solution
    COMMAND $<TARGET_FILE:lcom> run --headless --rtc 2026-06-16T12:34:56 -- $<TARGET_FILE:lab1_solution_test>
  )
  add_test(NAME lab2_solution
    COMMAND $<TARGET_FILE:lcom> run --headless -- $<TARGET_FILE:lab2_solution_test>
  )
  add_test(NAME lab3_solution
    COMMAND $<TARGET_FILE:lcom> run --headless --script ${CMAKE_CURRENT_SOURCE_DIR}/scripts/type_a_esc.lcomscript -- $<TARGET_FILE:lab3_solution_test>
  )
  add_test(NAME lab4_solution
    COMMAND $<TARGET_FILE:lcom> run --headless --script ${CMAKE_CURRENT_SOURCE_DIR}/scripts/mouse_move.lcomscript -- $<TARGET_FILE:lab4_solution_test>
  )
  add_test(NAME lab5_solution
    COMMAND $<TARGET_FILE:lcom> run --headless --dump-frame ${CMAKE_BINARY_DIR}/test-output/lab5-solution.ppm -- $<TARGET_FILE:lab5_solution_test>
  )
  add_test(NAME lab6_solution
    COMMAND $<TARGET_FILE:lcom> run --headless --audio-wav ${CMAKE_BINARY_DIR}/test-output/lab6-solution.wav -- $<TARGET_FILE:lab6_solution_test>
  )
  add_test(NAME lab7_solution
    COMMAND $<TARGET_FILE:lcom> run --headless -- $<TARGET_FILE:lab7_solution_test>
  )
  add_test(NAME integration
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.sh
      $<TARGET_FILE_DIR:lcom>
      ${CMAKE_BINARY_DIR}/examples
      ${CMAKE_BINARY_DIR}/test-output
  )
  set_tests_properties(integration PROPERTIES
    DEPENDS unit
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()
