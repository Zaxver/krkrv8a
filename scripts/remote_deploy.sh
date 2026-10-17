#!/bin/bash
# krkrv8a Remote Deploy Script
# This script is run from the workspace to deploy/update krkrv8a on Termux via SSH
# Usage: ./remote_deploy.sh [command]
# Commands: setup, update, build, shortcut, all

set -e

SSH_HOST="u0_a304"
SSH_IP="192.168.31.194"
SSH_PORT="8022"
SSH_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10"
SSH_CMD="ssh ${SSH_OPTS} -p ${SSH_PORT} ${SSH_HOST}@${SSH_IP}"
KRKR_REPO="https://github.com/Zaxver/krkrv8a.git"

echo "=== krkrv8a Remote Deploy ==="
echo "Target: ${SSH_HOST}@${SSH_IP}:${SSH_PORT}"

case "${1:-all}" in
    setup)
        echo "[1/4] Adding SSH key..."
        ${SSH_CMD} 'mkdir -p ~/.ssh && chmod 700 ~/.ssh' 2>/dev/null
        # Copy our pubkey
        cat ~/.ssh/id_ed25519.pub | ${SSH_CMD} 'cat >> ~/.ssh/authorized_keys 2>/dev/null; chmod 600 ~/.ssh/authorized_keys' 2>/dev/null
        echo "[OK] SSH key added"

        echo "[2/4] Starting sshd..."
        ${SSH_CMD} 'pgrep sshd >/dev/null || sshd; echo sshd_ok' 2>/dev/null
        echo "[OK] sshd running"

        echo "[3/4] Installing packages..."
        ${SSH_CMD} 'pkg update -y 2>/dev/null; pkg install -y git cmake clang make sdl2 sdl2-image sdl2-mixer sdl2-ttf freetype libpng libjpeg-turbo libvorbis libogg zlib pkg-config 2>/dev/null; echo packages_done' 2>/dev/null
        echo "[OK] Packages installed"

        echo "[4/4] Cloning repo..."
        ${SSH_CMD} "rm -rf ~/krkrv8a && git clone ${KRKR_REPO} ~/krkrv8a 2>/dev/null; echo clone_done" 2>/dev/null
        echo "[OK] Repository cloned"
        ;;

    update)
        echo "[1/2] Pulling latest code..."
        ${SSH_CMD} 'cd ~/krkrv8a && git pull origin main 2>/dev/null; echo pull_done' 2>/dev/null
        echo "[OK] Code updated"

        echo "[2/2] Building..."
        ${SSH_CMD} 'cd ~/krkrv8a && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5 && make -j$(nproc) 2>&1 | tail -10; echo build_done' 2>/dev/null
        echo "[OK] Build complete"
        ;;

    build)
        echo "Building krkrv8a..."
        ${SSH_CMD} 'cd ~/krkrv8a/build && make -j$(nproc) 2>&1 | tail -20; echo build_done' 2>/dev/null
        echo "[OK] Build complete"
        ;;

    shortcut)
        echo "Creating desktop shortcut..."
        ${SSH_CMD} 'bash ~/krkrv8a/scripts/create_shortcut.sh 2>&1; echo shortcut_done' 2>/dev/null
        echo "[OK] Shortcut created"
        ;;

    all)
        echo "=== Full deployment ==="
        $0 setup
        $0 update
        $0 shortcut
        echo ""
        echo "=== Deployment Complete ==="
        ${SSH_CMD} 'ls -la ~/krkrv8a/build/krkrv8a 2>/dev/null && echo BINARY_OK || echo BINARY_MISSING' 2>/dev/null
        ;;

    *)
        echo "Usage: $0 {setup|update|build|shortcut|all}"
        exit 1
        ;;
esac