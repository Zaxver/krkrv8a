#!/bin/bash
# krkrv8a Termux Build Setup
# Run this in Termux to install build dependencies

echo "=== krkrv8a Termux Setup ==="

pkg update -y 2>/dev/null || true
pkg install -y git cmake clang make sdl2 sdl2-image sdl2-mixer sdl2-ttf \
    freetype libpng libjpeg-turbo libvorbis libogg zlib libiconv \
    pkg-config zenity 2>/dev/null || {
    echo "[WARN] Some packages may need manual installation"
}

echo ""
echo "=== Setup Complete ==="
echo "Build: cd ~/krkrv8a && mkdir build && cd build && cmake .. && make"