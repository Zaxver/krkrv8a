#!/bin/bash
# krkrv8a Desktop Shortcut Creator for Android/Termux
# Creates a .desktop file and launcher script

set -e

TERMUX_HOME="${HOME}"
KRKR_DIR="${TERMUX_HOME}/krkrv8a"
KRKR_BIN="${KRKR_DIR}/build/krkrv8a"

echo "=== Creating krkrv8a Desktop Shortcut ==="

# Create launcher script
cat > "${TERMUX_HOME}/krkrv8a_launch.sh" << 'LAUNCHER'
#!/bin/bash
# krkrv8a Launcher Script

KRKR_DIR="${HOME}/krkrv8a"
KRKR_BIN="${KRKR_DIR}/build/krkrv8a"

# Check if binary exists
if [ ! -f "${KRKR_BIN}" ]; then
    echo "krkrv8a binary not found. Building..."
    cd "${KRKR_DIR}"
    git pull origin main 2>/dev/null || true
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release 2>/dev/null
    make -j$(nproc) 2>/dev/null
fi

# Set up environment
export SDL_VIDEODRIVER=wayland
export LD_LIBRARY_PATH="${KRKR_DIR}/build:${LD_LIBRARY_PATH}"

# Run krkrv8a
if [ -f "${KRKR_BIN}" ]; then
    exec "${KRKR_BIN}"
else
    echo "Error: krkrv8a binary not found at ${KRKR_BIN}"
    echo "Please build manually: cd ${KRKR_DIR} && mkdir build && cd build && cmake .. && make"
    exit 1
fi
LAUNCHER

chmod +x "${TERMUX_HOME}/krkrv8a_launch.sh"

# Create desktop entry for Android launcher
# This goes to the shared storage for Android launchers
DESKTOP_FILE="/sdcard/Download/krkrv8a.desktop"

cat > "${DESKTOP_FILE}" << 'DESKTOP'
[Desktop Entry]
Version=1.0
Type=Application
Name=krkrv8a
Comment=Kirikiri2 Engine for ARMv8a Linux
Exec=sh -c 'cd $HOME && ./krkrv8a_launch.sh'
Icon=krkrv8a
Terminal=false
Categories=Game;
X-Desktop-File-Install-Version=0.24
DESKTOP

echo "[OK] Desktop shortcut created at: ${DESKTOP_FILE}"
echo "[OK] Launcher script at: ${TERMUX_HOME}/krkrv8a_launch.sh"

# Also create a Termux:Widget shortcut
TERMUX_SHORTCUT="${TERMUX_HOME}/.shortcuts"
mkdir -p "${TERMUX_SHORTCUT}"

cp "${TERMUX_HOME}/krkrv8a_launch.sh" "${TERMUX_SHORTCUT}/krkrv8a"

echo "[OK] Termux shortcut created at: ${TERMUX_SHORTCUT}/krkrv8a"
echo ""
echo "=== Shortcut Creation Complete ==="