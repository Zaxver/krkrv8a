cmake_minimum_required(VERSION 3.14)
project(krkrv8a VERSION 1.0.0 LANGUAGES C CXX)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
option(KRKR_BUILD_LAUNCHER "Build the GUI launcher" ON)
option(KRKR_BUILD_CORE     "Build the engine core library" ON)
option(KRKR_BUILD_PLUGINS  "Build built-in plugins" ON)
option(KRKR_USE_GLES       "Use OpenGL ES instead of desktop GL" OFF)
option(KRKR_EMBED_IMGUI    "Embed Dear ImGui source" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------------------------------------------------------
# Architecture detection
# ----------------------------------------------------------------------------
execute_process(
    COMMAND uname -m
    RESULT_VARIABLE UNAME_RESULT
    OUTPUT_VARIABLE KRKR_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Target architecture: ${KRKR_ARCH}")

if(KRKR_ARCH STREQUAL "aarch64")
    set(KRKR_ARM64 ON)
    add_compile_definitions(KRKR_ARM64 __ARM_ARCH_8A__)
    if(KRKR_USE_GLES)
        add_compile_definitions(KRKR_USE_GLES)
    endif()
endif()

# ----------------------------------------------------------------------------
# Dependencies: SDL2 and friends
# ----------------------------------------------------------------------------
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Freetype
find_package(Freetype REQUIRED)

# zlib, libpng, libjpeg
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

# Ogg/Vorbis
find_package(Ogg REQUIRED)
find_package(Vorbis REQUIRED)

# Threads
find_package(Threads REQUIRED)

# ----------------------------------------------------------------------------
# Global compile definitions
# ----------------------------------------------------------------------------
add_compile_definitions(
    HAVE_CONFIG_H
    TVP_LOG_TO_COMMANDLINE_CONSOLE
    _GNU_SOURCE
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wno-non-virtual-dtor
        -fno-delete-null-pointer-checks
        -ffunction-sections
        -fdata-sections
    )
    add_link_options(-Wl,--gc-sections)
endif()

# ----------------------------------------------------------------------------
# ImGui (embedded)
# ----------------------------------------------------------------------------
if(KRKR_EMBED_IMGUI)
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui CACHE PATH "ImGui source dir")
endif()

# ----------------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/base
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/visual
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sound
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/environ
    ${CMAKE_CURRENT_SOURCE_DIR}/src/launcher
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
)

# ----------------------------------------------------------------------------
# Subdirectories
# ----------------------------------------------------------------------------
add_subdirectory(src/core)
add_subdirectory(src/launcher)
add_subdirectory(data)

# ----------------------------------------------------------------------------
# Final executable
# ----------------------------------------------------------------------------
set(KRKR_SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${KRKR_SOURCES})
target_link_libraries(${PROJECT_NAME}
    krkr_launcher
    krkr_core
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    Threads::Threads
    dl
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/krkrv8a)

# ----------------------------------------------------------------------------
# Version generation
# ----------------------------------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
    @ONLY
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)