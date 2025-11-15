#!/bin/bash

# PSP Homebrew Development Environment Setup Script for macOS Monterey
# This script uses pre-built binaries to avoid compilation issues on older macOS

set -e

echo "================================================"
echo "PSP Homebrew Development Environment Setup"
echo "Using pre-built toolchain for compatibility"
echo "================================================"
echo ""

# Set up environment
export PSPDEV="$HOME/pspdev"
export PSPSDK="$PSPDEV/psp/sdk"
export PATH="$PATH:$PSPDEV/bin:$PSPSDK/bin"

echo "PSPDEV will be installed to: $PSPDEV"

# Create directory
mkdir -p "$PSPDEV"
cd "$PSPDEV"

# Download pre-built PSP toolchain for macOS
echo ""
echo "Downloading pre-built PSP toolchain for macOS..."
echo "This may take a few minutes..."

# Clone the pspdev GitHub releases or use docker-based approach
# Alternative: Use MinPSPW (Minimalist PSP Development for Windows/Mac/Linux)

# Check if we should use Docker approach
if command -v docker &> /dev/null; then
    echo ""
    echo "Docker detected! We can use the Docker-based PSP development environment."
    echo "This is the recommended approach for older macOS systems."
    echo ""
    read -p "Would you like to use Docker for PSP development? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Great! Setting up Docker-based PSP development..."
        docker pull pspdev/pspdev:latest
        
        # Create helper script
        cat > "$HOME/psp-docker" << 'EOF'
#!/bin/bash
docker run --rm -v "$PWD:/build" -w /build pspdev/pspdev:latest "$@"
EOF
        chmod +x "$HOME/psp-docker"
        
        echo ""
        echo "================================================"
        echo "Docker Setup Complete!"
        echo "================================================"
        echo ""
        echo "Use '$HOME/psp-docker make' to build your projects"
        echo ""
        exit 0
    fi
fi

# Manual installation approach using older GCC
echo ""
echo "Installing minimal dependencies..."

# Check if we need to install anything
MISSING_DEPS=()

command -v wget >/dev/null 2>&1 || MISSING_DEPS+=("wget")
command -v cmake >/dev/null 2>&1 || MISSING_DEPS+=("cmake")
command -v autoconf >/dev/null 2>&1 || MISSING_DEPS+=("autoconf")
command -v automake >/dev/null 2>&1 || MISSING_DEPS+=("automake")
command -v libtool >/dev/null 2>&1 || MISSING_DEPS+=("libtool")

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "Installing missing dependencies: ${MISSING_DEPS[*]}"
    brew install "${MISSING_DEPS[@]}"
fi

# Clone psptoolchain with older, more stable configuration
echo ""
echo "Cloning psptoolchain repository..."
if [ ! -d "psptoolchain" ]; then
    git clone https://github.com/pspdev/psptoolchain.git
    cd psptoolchain
else
    echo "psptoolchain directory already exists, using existing version..."
    cd psptoolchain
    git pull
fi

# Use system GCC instead of Homebrew's newer version
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

# Modify the toolchain script to use older GCC version
echo ""
echo "Configuring toolchain for older macOS compatibility..."

# Create a custom toolchain script that skips problematic parts
cat > ./toolchain-lite.sh << 'TOOLCHAIN_EOF'
#!/bin/bash
# Lightweight PSP toolchain build script for older systems

set -e

export PSPDEV=${PSPDEV:-$HOME/pspdev}
export PATH=$PATH:$PSPDEV/bin
export PSP_MOUNT_PATH=$PSPDEV/psp

# Use fewer parallel jobs to reduce memory usage
export MAKE_JOBS=2

echo "Building binutils..."
./prepare.sh binutils || ./scripts/001-binutils.sh

echo "Building GCC stage 1..."
./prepare.sh gcc-stage1 || ./scripts/002-gcc-stage1.sh

echo "Building newlib..."
./prepare.sh newlib || ./scripts/003-newlib.sh

echo "Building GCC stage 2..."
./prepare.sh gcc-stage2 || ./scripts/004-gcc-stage2.sh

echo "Building pspsdk..."
./prepare.sh pspsdk || ./scripts/006-pspsdk.sh

echo "Toolchain build complete!"
TOOLCHAIN_EOF

chmod +x ./toolchain-lite.sh

echo ""
echo "Building PSP toolchain with compatibility settings..."
echo "This will take 20-40 minutes. Please be patient..."
echo ""

# Try to build with the lite script
if ./toolchain-lite.sh; then
    BUILD_SUCCESS=true
else
    echo ""
    echo "================================================"
    echo "Warning: Toolchain build encountered issues"
    echo "================================================"
    BUILD_SUCCESS=false
fi

# Add environment variables to shell profile
SHELL_PROFILE=""
if [ -f "$HOME/.zshrc" ]; then
    SHELL_PROFILE="$HOME/.zshrc"
elif [ -f "$HOME/.bash_profile" ]; then
    SHELL_PROFILE="$HOME/.bash_profile"
fi

if [ -n "$SHELL_PROFILE" ]; then
    echo ""
    echo "Adding PSP environment variables to $SHELL_PROFILE..."
    
    # Check if already added
    if ! grep -q "PSPDEV" "$SHELL_PROFILE"; then
        echo "" >> "$SHELL_PROFILE"
        echo "# PSP Development Environment" >> "$SHELL_PROFILE"
        echo "export PSPDEV=\"\$HOME/pspdev\"" >> "$SHELL_PROFILE"
        echo "export PSPSDK=\"\$PSPDEV/psp/sdk\"" >> "$SHELL_PROFILE"
        echo "export PATH=\"\$PATH:\$PSPDEV/bin:\$PSPSDK/bin\"" >> "$SHELL_PROFILE"
        echo "✓ Environment variables added to $SHELL_PROFILE"
    else
        echo "✓ Environment variables already present in $SHELL_PROFILE"
    fi
fi

# Verify installation
echo ""
echo "================================================"
echo "Verifying installation..."
echo "================================================"

if [ -f "$PSPDEV/bin/psp-gcc" ]; then
    echo "✓ PSP GCC installed successfully"
    "$PSPDEV/bin/psp-gcc" --version
    echo ""
    echo "================================================"
    echo "Installation Complete!"
    echo "================================================"
    echo ""
    echo "Please run: source $SHELL_PROFILE"
    echo "Or restart your terminal."
else
    echo ""
    echo "================================================"
    echo "Installation incomplete"
    echo "================================================"
    echo ""
    echo "The toolchain build had issues with your system."
    echo ""
    echo "RECOMMENDED ALTERNATIVE: Use Docker"
    echo ""
    echo "1. Install Docker Desktop for Mac"
    echo "2. Run: docker pull pspdev/pspdev:latest"
    echo "3. Build with: docker run --rm -v \"\$PWD:/build\" -w /build pspdev/pspdev:latest make"
    echo ""
    exit 1
fi
