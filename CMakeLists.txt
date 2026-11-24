
cmake_minimum_required(VERSION 3.16)

project(PiBreeze
    VERSION 1.0.0
    DESCRIPTION "Native Raspberry Pi System Monitor"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# GTK4 dependency
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

# Application executable
add_executable(pibreeze
    src/main.c
    src/thermal.c
    src/fan.c
    src/system_info.c
    src/ui/window.c
)

target_include_directories(pibreeze PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(pibreeze PRIVATE
    PkgConfig::GTK4
    m
)

# Compiler warnings
target_compile_options(pibreeze PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Installation
install(TARGETS pibreeze
    RUNTIME DESTINATION bin
)
