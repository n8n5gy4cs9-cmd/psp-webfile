# PSP Web File Browser - Implementation Summary

## What Was Built

A complete PSP homebrew application with two main features:

### 1. Web Page Viewer
- Fetches and displays content from http://softa.site/psp
- Shows the first 25 lines of the page content
- Simple text-based display

### 2. File Browser & Downloader
- Fetches file list from http://softa.site/pspfiles (PHP endpoint)
- Displays up to 100 files in a scrollable list
- Allows user to select and download files
- Saves files directly to ms0:/PSP/GAME/

## Main Components

### PSP Application (main.c)
- **Menu System**: Three states (Main Menu, Web View, File List)
- **Network Initialization**: Automatic WiFi connection on startup
- **HTTP Client**: Simple HTTP GET implementation
- **File Parser**: Parses newline-separated file lists
- **Download Manager**: Fetches and saves files to memory stick
- **UI**: Text-based interface using pspDebugScreen

### Server Files
- **psp.html**: Web page displayed in view #1
- **pspfiles.php**: Returns list of available files (one per line)
- **files/**: Directory containing downloadable files
- **.htaccess**: URL rewriting for clean endpoints
- **README.md**: Server setup instructions

## Key Features

1. **Automatic Network Connection**
   - Initializes network modules on startup
   - Connects to configured access point (Connection 1)
   - Shows connection status

2. **HTTP Client**
   - DNS resolution support
   - Socket-based HTTP GET requests
   - Response parsing (extracts body from headers)

3. **Navigation**
   - D-Pad Up/Down for menu navigation
   - X button to select/download
   - Circle button to go back/exit

4. **File Management**
   - Scrollable file list (20 visible at a time)
   - Direct download to memory stick
   - Visual feedback for downloads

## File Structure

```
psp-webfile/
├── main.c                    # Main application code
├── Makefile                  # Top-level build wrapper
├── Makefile.base             # Actual build configuration
├── USERGUIDE.md              # User documentation
├── README.md                 # Project overview
├── build/                    # Build output directory
│   └── EBOOT.PBP            # Final PSP executable
└── server/                   # Server-side files
    ├── psp.html             # Web page for view #1
    ├── pspfiles.php         # File list endpoint
    ├── .htaccess            # Apache configuration
    ├── README.md            # Server setup guide
    └── files/               # Directory for downloadable files
```

## How It Works

### View 1: Web Page
1. User selects "View Web Page" from menu
2. App sends HTTP GET to http://softa.site/psp
3. Receives HTML response
4. Extracts body (skips HTTP headers)
5. Displays first 25 lines

### View 2: File Browser
1. User selects "Browse & Download Files"
2. App sends HTTP GET to http://softa.site/pspfiles
3. PHP script returns file list (one filename per line)
4. App parses list and displays in scrollable view
5. User navigates with D-Pad, selects file with X
6. App downloads file from http://softa.site/pspfiles/[filename]
7. Saves to ms0:/PSP/GAME/[filename]

## Network Stack

```
Application Layer:    HTTP GET requests
Transport Layer:      TCP sockets (sceNetInet*)
Network Layer:        IP + DNS (sceNetResolver*)
Link Layer:           WiFi (sceNetApctl*)
Physical Layer:       PSP WiFi hardware
```

## Build Configuration

### Libraries Linked
- `-lpsppower` - Power management
- `-lpspnet` - Network initialization
- `-lpspnet_inet` - Internet protocols
- `-lpspnet_apctl` - Access point control
- `-lpspnet_resolver` - DNS resolution
- `-lpsputility` - Utility functions

### Compiler Flags
- `-O2` - Optimization level 2
- `-G0` - No small data section
- `-Wall` - All warnings

## Server Requirements

### Web Server
- Apache or Nginx with PHP support
- PHP 5.4+ (for scandir, is_dir, etc.)
- Write access to create files/ directory

### Directory Structure
```
/var/www/softa.site/
├── psp.html          # or configured via rewrite
├── pspfiles.php      # or configured as /pspfiles
└── files/            # Contains downloadable files
    ├── game1.pbp
    ├── theme1.zip
    └── ...
```

### URL Endpoints
- `http://softa.site/psp` → psp.html
- `http://softa.site/pspfiles` → pspfiles.php (returns file list)
- `http://softa.site/pspfiles/[filename]` → files/[filename]

## Testing

### PSP Testing
1. Configure WiFi on PSP (Settings → Network Settings)
2. Copy EBOOT.PBP to ms0:/PSP/GAME/PSPWebFile/
3. Launch from Game menu
4. Should auto-connect and show main menu

### Server Testing
1. Test http://softa.site/psp in web browser
2. Test http://softa.site/pspfiles in web browser (should show file list)
3. Add test files to files/ directory
4. Verify files appear in list
5. Test file download: http://softa.site/pspfiles/testfile.txt

## Future Enhancements (Potential)

- HTML rendering (basic tag stripping for better readability)
- Download progress indicators
- File size display
- Multiple page support in web viewer
- Bookmarks/favorites
- File manager to view downloaded files
- Delete downloaded files
- HTTPS support (if PSP supports it)
- POST request support
- Form submission
- Image display

## Notes

- Maximum 100 files in list (configurable via MAX_FILES)
- 20 files visible at once (scrollable)
- First 25 lines shown for web pages
- Files saved directly to ms0:/PSP/GAME/
- Uses Connection 1 from PSP network settings
- Text-only display (no graphics rendering)
- Simple HTTP/1.1 implementation
- No SSL/TLS support
- No cookies or session management
- Single-threaded synchronous operations

## Building

```bash
# Clean build
make clean

# Build application
make

# Output
build/EBOOT.PBP
```

## Deployment

### PSP
1. Copy build/EBOOT.PBP to PSP
2. Place in ms0:/PSP/GAME/PSPWebFile/EBOOT.PBP

### Server
1. Upload server/ files to web host
2. Configure web server (use .htaccess or nginx config)
3. Create files/ directory
4. Add files to make available
5. Test endpoints in browser

## Success Criteria ✓

- [x] Main menu with 2 options
- [x] View web page from http://softa.site/psp
- [x] Fetch file list from http://softa.site/pspfiles
- [x] User can select files from list
- [x] User can download selected files
- [x] Files saved to PSP memory stick
- [x] Network initialization
- [x] D-Pad navigation
- [x] Visual feedback for operations
- [x] Server-side PHP script
- [x] Server-side web page
- [x] Documentation
