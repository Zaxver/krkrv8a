cmake_minimum_required(VERSION 3.14)

# Core engine library - only real source files (no empty stubs)
set(KRKR_CORE_SOURCES
    base/tvp_types.cpp
    base/CharacterSet.cpp
    base/BinaryStream.cpp
    base/XP3Archive.cpp
    base/PFSArchive.cpp
    base/StorageImpl.cpp
    base/EventImpl.cpp
    visual/LayerImpl.cpp
    sound/WaveImpl.cpp
    environ/Application.cpp
    environ/Platform.cpp
    environ/SystemControl.cpp
)

add_library(krkr_core STATIC ${KRKR_CORE_SOURCES})

target_include_directories(krkr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/base
    ${CMAKE_CURRENT_SOURCE_DIR}/visual
    ${CMAKE_CURRENT_SOURCE_DIR}/sound
    ${CMAKE_CURRENT_SOURCE_DIR}/environ
)

target_link_libraries(krkr_core PUBLIC
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