#!/bin/bash

# PSP Homebrew Development Environment Setup Script for macOS Monterey
# This script installs the PSPSDK and toolchain for PSP homebrew development

set -e

echo "================================================"
echo "PSP Homebrew Development Environment Setup"
echo "================================================"
echo ""

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
else
    echo "✓ Homebrew is already installed"
fi

# Update Homebrew
echo ""
echo "Updating Homebrew..."
brew update

# Install required dependencies
echo ""
echo "Installing dependencies..."
brew install wget cmake autoconf automake libtool gcc make doxygen

# Set up environment
export PSPDEV="$HOME/pspdev"
export PSPSDK="$PSPDEV/psp/sdk"
export PATH="$PATH:$PSPDEV/bin:$PSPSDK/bin"

echo ""
echo "PSPDEV will be installed to: $PSPDEV"

# Create directory
mkdir -p "$PSPDEV"
cd "$PSPDEV"

# Clone psptoolchain
echo ""
echo "Cloning psptoolchain repository..."
if [ ! -d "psptoolchain" ]; then
    git clone https://github.com/pspdev/psptoolchain.git
    cd psptoolchain
else
    echo "psptoolchain directory already exists, updating..."
    cd psptoolchain
    git pull
fi

# Run the toolchain script
echo ""
echo "Building PSP toolchain (this may take 30-60 minutes)..."
echo "Please be patient..."
./toolchain.sh

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
else
    echo "✗ PSP GCC not found. Installation may have failed."
    exit 1
fi

echo ""
echo "================================================"
echo "Installation Complete!"
echo "================================================"
echo ""
echo "Please run the following command to apply environment variables:"
echo "  source $SHELL_PROFILE"
echo ""
echo "Or simply restart your terminal."
echo ""
echo "You can now start developing PSP homebrew applications!"
