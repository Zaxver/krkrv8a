#!/bin/bash
# krkrv8a Termux Deployment Script
# Run this in Termux to set up SSH access and build environment

set -e

echo "=== krkrv8a Termux Setup ==="

# Step 1: Add workspace public key to authorized_keys
PUBKEY="ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIJ9/Ew/hmx+kgUFSL2P72JAoQb6GYZuG1Jnb1JCadVeQ workspace-to-termux"

mkdir -p ~/.ssh
chmod 700 ~/.ssh

# Check if key already exists
if grep -q "IJ9/Ew/hmx" ~/.ssh/authorized_keys 2>/dev/null; then
    echo "[OK] SSH key already authorized"
else
    echo "$PUBKEY" >> ~/.ssh/authorized_keys
    echo "[OK] SSH key added to authorized_keys"
fi
chmod 600 ~/.ssh/authorized_keys

# Start sshd if not running
if ! pgrep sshd >/dev/null 2>&1; then
    echo "[INFO] Starting sshd..."
    sshd
    echo "[OK] sshd started"
else
    echo "[OK] sshd already running"
fi

# Step 2: Install build dependencies
echo ""
echo "=== Installing build dependencies ==="

pkg update -y 2>/dev/null || true
pkg install -y git cmake clang make sdl2 sdl2-image sdl2-mixer sdl2-ttf \
    freetype libpng libjpeg-turbo libvorbis libogg zlib libiconv \
    pkg-config 2>/dev/null || {
    echo "[WARN] Some packages may need manual installation"
    echo "Run: pkg install git cmake clang make sdl2 sdl2-image sdl2-mixer sdl2-ttf freetype libpng libjpeg-turbo libvorbis libogg zlib"
}

echo ""
echo "=== Setup Complete ==="
echo "SSH: ssh -p 8022 u0_a304@$(ip addr show wlan0 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)"
echo ""
echo "You can now build krkrv8a with:"
echo "  git clone https://github.com/Zaxver/krkrv8a.git ~/krkrv8a"
echo "  cd ~/krkrv8a && mkdir build && cd build"
echo "  cmake .. -DCMAKE_BUILD_TYPE=Release"
echo "  make -j$(nproc)"