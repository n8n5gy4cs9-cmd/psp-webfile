# Sample Files for PSP Web File Browser

Place files you want to make available for download in this directory.

## Supported File Types

The PSP can handle various file types:

- **.pbp** - PSP applications (homebrews, games)
- **.zip** - Compressed archives
- **.txt** - Text files
- **.jpg, .png, .gif** - Images
- **.mp3** - Audio files
- **.mp4** - Video files
- **.iso, .cso** - Game images (if custom firmware)

## Example Files

Create some test files:

```bash
# Create test files
echo "This is a test text file" > test.txt
echo "Sample readme content" > README.txt
```

## File Organization

You can organize files however you like. The PHP script will list all files in this directory (non-recursive).

For example:
```
files/
├── homebrew-app.pbp
├── cool-theme.zip
├── wallpaper.jpg
├── readme.txt
└── music.mp3
```

## File Naming

- Use descriptive names
- Avoid special characters
- Keep names reasonably short
- Use lowercase for compatibility

## Size Considerations

- PSP memory sticks vary (32MB to 32GB+)
- Be mindful of file sizes
- Consider PSP's download speed (WiFi b/g)
- Test with smaller files first

## Sample Content Ideas

- PSP homebrew applications
- Game saves
- Themes and wallpapers
- Music files
- Video files
- Documentation (TXT files)
- Emulator ROMs (where legal)
- ISO/CSO games (with appropriate rights)
