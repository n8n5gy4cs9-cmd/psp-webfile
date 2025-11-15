#!/bin/bash
# Build PSP Web File Browser using Docker

echo "Building PSP Web File Browser with Docker..."
echo ""

# Clean first
echo "Cleaning previous build..."
/home/codespace/psp-docker make clean 2>/dev/null || echo "Nothing to clean"

# Build
echo ""
echo "Building application..."
/home/codespace/psp-docker make

# Check if build succeeded
if [ -f "build/EBOOT.PBP" ]; then
    echo ""
    echo "================================================"
    echo "✓ Build successful!"
    echo "================================================"
    echo ""
    echo "Output: build/EBOOT.PBP"
    echo ""
    ls -lh build/EBOOT.PBP
    echo ""
    echo "Next steps:"
    echo "1. Download EBOOT.PBP (right-click → Download)"
    echo "2. Copy to PSP: ms0:/PSP/GAME/PSPWebFile/EBOOT.PBP"
    echo "3. Launch from PSP Game menu"
    echo ""
else
    echo ""
    echo "Build failed. Check errors above."
    exit 1
fi
