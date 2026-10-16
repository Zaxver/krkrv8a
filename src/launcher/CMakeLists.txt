cmake_minimum_required(VERSION 3.14)

set(KRKR_LAUNCHER_SOURCES
    Launcher.cpp
    Launcher.h
)

add_library(krkr_launcher STATIC ${KRKR_LAUNCHER_SOURCES})

target_include_directories(krkr_launcher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(krkr_launcher PUBLIC
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    krkr_core
)