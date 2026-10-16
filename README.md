# krkrv8a - Kirikiri2 Engine for ARMv8a Linux

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ARM64 Linux](https://img.shields.io/badge/Platform-ARM64%20Linux-blue.svg)](https://github.com/Zaxver/krkrv8a)

A lightweight Kirikiri2/TVP2-compatible visual novel engine designed to run **non-PC galgames** on **ARMv8a (ARM64) Linux** devices.

## Features

- 🎮 **Full Kirikiri2 engine compatibility** - Runs `.xp3`, `.kgt` packaged games
- 🖥️ **Custom GUI launcher** - Double-click to open, browse and launch games
- 📁 **Game library management** - Add game folders, launch with double-click
- 🔧 **ARM64 native** - Built for aarch64 Linux with hardware acceleration
- 🎵 **Audio support** - WAV, Ogg Vorbis, MP3 playback via SDL2_mixer
- 🖼️ **Graphics** - OpenGL ES 3.0 rendering via SDL2
- 💾 **Save/Load** - Full save state support
- 🌐 **Cross-platform core** - Based on SDL2, portable to many platforms

## Building

### Prerequisites (Debian/Ubuntu ARM64)

```bash
sudo apt install -y build-essential cmake git \
    libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev \
    libfreetype6-dev libpng-dev libjpeg-dev libvorbis-dev libogg-dev \
    libzip-dev zlib1g-dev libcurl4-openssl-dev
```

### Build Instructions

```bash
git clone https://github.com/Zaxver/krkrv8a.git
cd krkrv8a
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

The executable `krkrv8a` will be in `build/`.

## Usage

1. **Double-click** the `krkrv8a` executable to launch the GUI
2. Click **"Add Game"** to add a game folder or XP3 file
3. **Double-click** a game in the list to launch it
4. The game opens in a new window

### Command Line

```bash
# Launch GUI
./krkrv8a

# Launch directly with a game path
./krkrv8a /path/to/game.xp3
./krkrv8a /path/to/game/folder/
```

## Architecture

```
krkrv8a/
├── src/
│   ├── core/           # Kirikiri2 engine core
│   │   ├── base/       # Base system, events, storage, scripting
│   │   ├── visual/     # Rendering, layers, bitmaps
│   │   ├── sound/      # Audio playback
│   │   └── environ/    # Platform abstraction (SDL2)
│   ├── launcher/       # GUI launcher (Dear ImGui)
│   └── plugins/        # Engine plugins
├── data/               # Engine data files (startup.tjs, etc.)
├── assets/             # Launcher assets (icons, fonts)
├── cmake/              # CMake helper modules
├── scripts/            # Build/utility scripts
└── docs/               # Documentation
```

## References

This project is inspired by and references code from:
- [krkrsdl2](https://github.com/krkrsdl2/krkrsdl2) - SDL2 backend for Kirikiri2
- [krkr2-linux](https://github.com/francistw/krkr2-linux) - Cocos2d-x based Kirikiri2 for Linux
- [Kirikiroid2](https://github.com/zeas2/Kirikiroid2) - Android Kirikiri2 port
- [tyranor](https://github.com/tyranor2/tyranor) - Kirikiri2 fork for web/mobile

## License

MIT License - See [LICENSE](LICENSE) for details.
