cmake_minimum_required(VERSION 3.16)
project(csi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OS_SOURCE src/os/terminal_os_posix.cpp)
if(WIN32)
    set(OS_SOURCE src/os/terminal_os_windows.cpp)
endif()

add_executable(csi
    src/main.cpp
    src/commands/command.cpp
    src/commands/fill_command.cpp
    src/commands/lava_command.cpp
    src/commands/palette256_command.cpp
    src/terminal/csi.cpp
    src/terminal/terminal.cpp
    src/terminal/terminal16.cpp
    src/terminal/terminal256.cpp
    src/terminal/terminalRGB.cpp
    src/os/terminal_os_signal.cpp
    ${OS_SOURCE}
)

target_include_directories(csi PRIVATE src src/include src/os)

if(MSVC)
    target_compile_options(csi PRIVATE /W4)
else()
    target_compile_options(csi PRIVATE -Wall -Wextra -Wpedantic)
endif()
