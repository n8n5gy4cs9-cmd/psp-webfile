#!/usr/bin/env bash
set -euo pipefail

echo "[devcontainer-setup] Starting PSP toolchain setup..."

# Installation prefix inside the container
PSPDEV=${PSPDEV:-/opt/pspdev}
PSPTC_DIR="$PSPDEV/psptoolchain"
PSPSDK="$PSPDEV/psp/sdk"
JOBS=${TOOLCHAIN_JOBS:-2}

export PSPDEV PSPSDK

mkdir -p "$PSPDEV"

if command -v psp-gcc >/dev/null 2>&1; then
    echo "[devcontainer-setup] psp-gcc already present. Skipping toolchain build."
    exit 0
fi

if [ -d "$PSPTC_DIR" ]; then
    echo "[devcontainer-setup] psptoolchain directory exists. Updating repository..."
    cd "$PSPTC_DIR"
    git fetch --all --tags --prune
    git pull --rebase || true
else
    echo "[devcontainer-setup] Cloning psptoolchain into $PSPTC_DIR"
    git clone https://github.com/pspdev/psptoolchain.git "$PSPTC_DIR"
fi

cd "$PSPTC_DIR"

echo "[devcontainer-setup] Running toolchain build (this can take a long time). Jobs=$JOBS"
# Try to run toolchain.sh; if it fails, provide a helpful message but do not abort codespace creation
if ./toolchain.sh -j "$JOBS"; then
    echo "[devcontainer-setup] Toolchain build completed successfully."
else
    echo "[devcontainer-setup] Toolchain build failed or exited non-zero. You can retry inside the container:"
    echo "  cd $PSPTC_DIR && ./toolchain.sh -j $JOBS"
fi

# Add environment exports for interactive shells
PROFILE_FILE="/etc/profile.d/pspdev.sh"
echo "[devcontainer-setup] Writing environment to $PROFILE_FILE"
cat > "$PROFILE_FILE" <<EOF
export PSPDEV="$PSPDEV"
export PSPSDK="$PSPSDK"
export PATH="\$PATH:$PSPDEV/bin:$PSPSDK/bin"
EOF

chmod 644 "$PROFILE_FILE" || true

echo "[devcontainer-setup] Setup complete. You may need to open a new terminal or source $PROFILE_FILE"
echo "[devcontainer-setup] Verify with: psp-gcc --version  (may require a new shell session)"

exit 0
