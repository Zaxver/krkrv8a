cmake_minimum_required(VERSION 3.14)
project(krkrv8a VERSION 1.0.0 LANGUAGES C CXX)

# Build options
option(KRKR_BUILD_LAUNCHER "Build the GUI launcher" ON)
option(KRKR_BUILD_CORE     "Build the engine core library" ON)
option(KRKR_USE_GLES       "Use OpenGL ES instead of desktop GL" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use pkg-config for all dependencies
find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(PNG REQUIRED libpng)
pkg_check_modules(JPEG REQUIRED libjpeg)
pkg_check_modules(VORBIS REQUIRED vorbis vorbisfile)
pkg_check_modules(OGG REQUIRED ogg)
pkg_check_modules(ZLIB REQUIRED zlib)

find_package(Threads REQUIRED)

# Architecture detection
execute_process(COMMAND uname -m OUTPUT_VARIABLE KRKR_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "Target architecture: ${KRKR_ARCH}")

if(KRKR_ARCH STREQUAL "aarch64")
    set(KRKR_ARM64 ON)
    add_compile_definitions(KRKR_ARM64 __ARM_ARCH_8A__)
endif()

add_compile_definitions(HAVE_CONFIG_H TVP_LOG_TO_COMMANDLINE_CONSOLE _GNU_SOURCE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-non-virtual-dtor -ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
endif()

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

add_subdirectory(src/core)
add_subdirectory(src/launcher)
add_subdirectory(data)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME}
    krkr_launcher krkr_core
    ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES}
    ${SDL2_TTF_LIBRARIES} ${FREETYPE_LIBRARIES} ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES} ${VORBIS_LIBRARIES} ${OGG_LIBRARIES}
    ${ZLIB_LIBRARIES} Threads::Threads dl
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/krkrv8a)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h @ONLY)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)