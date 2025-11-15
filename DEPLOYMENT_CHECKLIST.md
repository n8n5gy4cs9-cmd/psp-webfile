# PSP Web File Browser - Deployment Checklist

## PSP Side Checklist

### Prerequisites
- [ ] PSP with custom firmware (CFW) or homebrew enabled
- [ ] Memory stick (any size)
- [ ] WiFi access point available
- [ ] PSP WiFi configured (Connection 1)

### Installation Steps
- [ ] Build the application: `make clean && make`
- [ ] Verify `build/EBOOT.PBP` exists
- [ ] Create directory on PSP: `ms0:/PSP/GAME/PSPWebFile/`
- [ ] Copy `EBOOT.PBP` to `ms0:/PSP/GAME/PSPWebFile/EBOOT.PBP`
- [ ] Safely eject memory stick
- [ ] Insert memory stick into PSP

### PSP Configuration
- [ ] Go to Settings → Network Settings
- [ ] Configure Connection 1 with your WiFi
  - [ ] SSID (network name)
  - [ ] Security (WPA2-PSK recommended)
  - [ ] Password
- [ ] Test connection (should succeed)
- [ ] Save settings

### First Launch
- [ ] Go to Game → Memory Stick
- [ ] Find "PSP Web File Browser"
- [ ] Launch the application
- [ ] Wait for "Connecting to access point..."
- [ ] Should see "Connected!"
- [ ] Main menu should appear

---

## Server Side Checklist

### Prerequisites
- [ ] Web server (Apache or Nginx)
- [ ] PHP support (5.4+)
- [ ] Access to softa.site
- [ ] SSH/FTP access to server

### File Upload
- [ ] Upload `server/psp.html` to server
- [ ] Upload `server/pspfiles.php` to server
- [ ] Upload `server/.htaccess` to server (if Apache)
- [ ] Create `files/` directory
- [ ] Set permissions: `chmod 755 files/`

### Directory Structure
Verify this structure exists:
```
/var/www/softa.site/
├── psp.html
├── pspfiles.php
├── .htaccess
└── files/
    └── (your files here)
```

### URL Rewriting (Apache)
- [ ] Ensure `mod_rewrite` is enabled
- [ ] `.htaccess` is in web root
- [ ] Test: http://softa.site/psp (should show HTML page)
- [ ] Test: http://softa.site/pspfiles (should show file list)

### URL Rewriting (Nginx)
Add to nginx config:
```nginx
location /psp {
    try_files $uri /psp.html;
}

location = /pspfiles {
    try_files $uri /pspfiles.php?$args;
}

location ~ ^/pspfiles/(.+)$ {
    alias /var/www/softa.site/files/$1;
}
```

### Test Files
- [ ] Add test files to `files/` directory
- [ ] Example: `echo "Hello PSP" > files/test.txt`
- [ ] Verify files are readable: `ls -la files/`

### Web Testing
Test from a web browser:
- [ ] Visit http://softa.site/psp
  - [ ] Should show nicely formatted page
- [ ] Visit http://softa.site/pspfiles
  - [ ] Should show plain text list of files
  - [ ] One filename per line
- [ ] Visit http://softa.site/pspfiles/test.txt
  - [ ] Should download/display the file

---

## Testing Checklist

### Network Test (PSP)
- [ ] Launch app on PSP
- [ ] Wait for "Connected!" message
- [ ] Main menu shows "Network: Connected"

### Web View Test
- [ ] Select "View Web Page" from main menu
- [ ] Press X to select
- [ ] Wait for "Loading..." message
- [ ] Page content should appear (first 25 lines)
- [ ] Press Circle to return to menu

### File Browser Test
- [ ] Select "Browse & Download Files" from main menu
- [ ] Press X to select
- [ ] Wait for "Fetching file list..."
- [ ] File list should appear
- [ ] Files should match what's in server's files/ directory

### Download Test
- [ ] Navigate file list with D-Pad Up/Down
- [ ] Select a small test file
- [ ] Press X to download
- [ ] Should see "Downloading [filename]..."
- [ ] Should see "Downloaded to ms0:/PSP/GAME/[filename]"
- [ ] Exit app and check PSP file browser
- [ ] File should exist in `ms0:/PSP/GAME/`

### Error Handling Test
- [ ] Turn off WiFi router
- [ ] Launch app
- [ ] Should see "Connection failed!" message
- [ ] Should still be usable (won't crash)
- [ ] Turn WiFi back on and try again

---

## Troubleshooting Checklist

### "Network initialization failed"
- [ ] Check PSP WiFi is enabled
- [ ] Verify at least one network connection is configured
- [ ] Check PSP system update (firmware)

### "Connection failed"
- [ ] Verify WiFi password is correct
- [ ] Check WiFi router is powered on
- [ ] Ensure router is in range
- [ ] Try connecting to WiFi from PSP Settings first
- [ ] Check router MAC filtering (if enabled)

### "Could not load page"
- [ ] Test http://softa.site/psp in a web browser
- [ ] Verify server is running
- [ ] Check DNS is resolving softa.site
- [ ] Verify .htaccess or nginx config is correct
- [ ] Check server firewall rules

### "No files found"
- [ ] Test http://softa.site/pspfiles in a web browser
- [ ] Verify pspfiles.php is accessible
- [ ] Check files/ directory exists
- [ ] Verify files/ contains files
- [ ] Check PHP is working on server

### "Could not save file"
- [ ] Check memory stick has free space
- [ ] Verify memory stick is not locked (physical switch)
- [ ] Check ms0:/PSP/GAME/ directory exists
- [ ] Try creating a file manually to test write access
- [ ] Format memory stick if necessary (backup first!)

### Downloads succeed but file is corrupt
- [ ] File may be binary (PBP, ZIP, etc)
- [ ] Check server is sending correct MIME type
- [ ] Try downloading in a web browser first
- [ ] Verify file on server is not corrupted

---

## Performance Checklist

### PSP Performance
- [ ] App starts in < 10 seconds
- [ ] Network connects in < 15 seconds
- [ ] Menu is responsive (no lag)
- [ ] File list loads in < 5 seconds
- [ ] Small files download quickly

### Server Performance
- [ ] psp.html loads in < 1 second
- [ ] pspfiles.php executes in < 1 second
- [ ] File downloads start immediately
- [ ] No 404 errors in server logs
- [ ] No PHP errors in server logs

---

## Security Checklist

### Server Security
- [ ] Files directory has appropriate permissions (755)
- [ ] PHP script validates file names
- [ ] No directory traversal possible
- [ ] `.htaccess` disables directory listing
- [ ] Consider rate limiting for downloads
- [ ] Monitor server logs for abuse

### PSP Security
- [ ] Only download from trusted sources
- [ ] Verify PBP files before running
- [ ] Don't run unknown homebrew
- [ ] Keep custom firmware updated
- [ ] Regular memory stick backups

---

## Maintenance Checklist

### Regular Tasks
- [ ] Monitor server disk space
- [ ] Check server logs for errors
- [ ] Update PHP if needed
- [ ] Test app after server changes
- [ ] Keep file list organized
- [ ] Remove old/unused files
- [ ] Update psp.html content as needed

### When Adding New Files
- [ ] Upload to files/ directory
- [ ] Test download from web browser
- [ ] Test download from PSP
- [ ] Verify file integrity after download
- [ ] Update any documentation if needed

---

## Success Criteria

All items should be checked ✓ for successful deployment:

**PSP:**
- ✓ App installed
- ✓ WiFi configured
- ✓ App launches
- ✓ Connects to network
- ✓ Main menu visible

**Server:**
- ✓ Files uploaded
- ✓ URLs working
- ✓ File list displays
- ✓ Files downloadable

**Testing:**
- ✓ Can view web page
- ✓ Can browse file list
- ✓ Can download files
- ✓ Files save correctly
- ✓ No errors or crashes

---

## Post-Deployment

### Optional Enhancements
- [ ] Add more files to server
- [ ] Customize psp.html with your content
- [ ] Add thumbnails for files
- [ ] Create file categories
- [ ] Add file descriptions
- [ ] Implement download statistics
- [ ] Create admin interface

### Community Sharing
- [ ] Share EBOOT.PBP with community
- [ ] Provide server setup guide
- [ ] Create video tutorial
- [ ] Answer user questions
- [ ] Collect feedback for improvements

---

**Date Completed:** _______________

**Deployed By:** _______________

**Notes:**
_____________________________________________________________
_____________________________________________________________
_____________________________________________________________
