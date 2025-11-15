# PSP Web File Browser - User Guide

## Overview

PSP Web File Browser is a homebrew application that brings web connectivity to your PSP. It allows you to:

1. **View Web Pages** - Display content from http://softa.site/psp
2. **Browse Files** - See a list of available files from the server
3. **Download Files** - Download files directly to your PSP memory stick (ms0:/PSP/GAME/)

## Features

- **Main Menu Navigation** - Easy-to-use menu system with D-Pad controls
- **HTTP Client** - Built-in HTTP GET support for fetching web content
- **File Browser** - Scrollable file list with up to 100 files
- **Download Manager** - Direct downloads to memory stick
- **Network Status** - Shows connection status in the UI
- **Auto-Connect** - Automatically connects to your configured access point

## Controls

### Main Menu
- **D-Pad Up/Down** - Navigate between menu options
- **X Button** - Select current option
- **Circle Button** - Exit application

### Web Page View
- **Circle Button** - Return to main menu

### File Browser
- **D-Pad Up/Down** - Navigate through file list
- **X Button** - Download selected file
- **Circle Button** - Return to main menu

## Setup Instructions

### PSP Configuration

1. **Configure Wi-Fi Access Point:**
   - Go to PSP Settings → Network Settings
   - Set up your Wi-Fi connection (Connection 1)
   - Test the connection to ensure it works

2. **Install the Application:**
   - Copy `EBOOT.PBP` to `ms0:/PSP/GAME/PSPWebFile/`
   - The application will appear in the Game menu

3. **Launch:**
   - Go to Game → Memory Stick
   - Select "PSP Web File Browser"
   - The app will initialize the network and connect automatically

### Server Configuration

See `server/README.md` for details on setting up the server side.

Quick setup:
1. Place `psp.html` at http://softa.site/psp
2. Place `pspfiles.php` at http://softa.site/pspfiles
3. Create a `files/` directory for downloadable content
4. Configure web server to serve files from the files/ directory

## How It Works

### View Web Page (Option 1)
1. Select "View Web Page" from main menu
2. App fetches content from http://softa.site/psp
3. First 25 lines of HTML/text are displayed
4. Press Circle to return to menu

### Browse & Download Files (Option 2)
1. Select "Browse & Download Files" from main menu
2. App fetches file list from http://softa.site/pspfiles
3. Use D-Pad to browse through available files
4. Press X on a file to download it
5. File is saved to ms0:/PSP/GAME/[filename]
6. Download status is shown on screen

## Technical Details

### Network Libraries Used
- `pspnet` - Network initialization
- `pspnet_inet` - Internet sockets
- `pspnet_apctl` - Access point control
- `pspnet_resolver` - DNS resolution
- `psputility` - Utility functions

### File Locations
- **Downloaded files:** `ms0:/PSP/GAME/`
- **Application:** `ms0:/PSP/GAME/PSPWebFile/EBOOT.PBP`

### Memory Requirements
- Heap: Variable (uses system default minus 1MB)
- Suitable for PSP-1000, 2000, 3000, Go, and E1000 models

### Network Requirements
- Wi-Fi access point configured in PSP settings
- Internet connection
- Access to http://softa.site

## Troubleshooting

### "Network initialization failed"
- Ensure PSP Wi-Fi is enabled
- Check that you have configured at least one network connection
- Try moving closer to your Wi-Fi access point

### "Connection failed"
- Verify your Wi-Fi password is correct in PSP settings
- Check that your router is working
- Ensure Connection 1 is properly configured

### "Could not load page" or "Download failed"
- Check that http://softa.site is accessible
- Verify the server is running and configured correctly
- Test the URLs in a web browser first

### "Could not save file"
- Ensure your memory stick has free space
- Check that ms0:/PSP/GAME/ directory exists
- Memory stick should be properly inserted

## Building from Source

### Requirements
- PSPSDK (PSP Software Development Kit)
- psp-gcc toolchain
- Make

### Build Commands
```bash
# Clean previous build
make clean

# Build the application
make

# Output: build/EBOOT.PBP
```

### Development Environment
This project includes `.devcontainer` configuration for GitHub Codespaces or VS Code Remote Containers.

## License

This is a homebrew application for educational purposes. Use at your own risk.

## Credits

Built with PSPSDK and the PSP homebrew community's tools and documentation.

## Version History

### v1.0 (Current)
- Initial release
- Main menu with two views
- Web page viewer
- File browser and downloader
- Automatic network connection
- HTTP GET support
- File list parsing
- Direct memory stick downloads
