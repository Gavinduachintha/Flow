
cmake_minimum_required(VERSION 3.16)

project(Flow
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
add_executable(flow
    src/main.c
    src/thermal.c
    src/fan.c
    src/system_info.c
    src/ui/window.c
)

target_include_directories(flow PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(flow PRIVATE
    PkgConfig::GTK4
    m
)

# Compiler warnings
target_compile_options(flow PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Installation
install(TARGETS flow
    RUNTIME DESTINATION bin
)

install(FILES assets/icons/Flow_128.png
    DESTINATION share/icons/hicolor/128x128/apps
    RENAME flow.png
)
install(FILES assets/icons/Flow_256.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME flow.png
)
install(FILES assets/icons/Flow_512.png
    DESTINATION share/icons/hicolor/512x512/apps
    RENAME flow.png
)
install(FILES flow.desktop
    DESTINATION share/applications
)
