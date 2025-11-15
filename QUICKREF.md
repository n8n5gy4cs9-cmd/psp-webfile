# PSP Web File Browser - Quick Reference

## PSP Controls

```
┌─────────────────────────────────────────┐
│        MAIN MENU                        │
├─────────────────────────────────────────┤
│  △ Up    : Previous menu item           │
│  ▽ Down  : Next menu item               │
│  ✕ Cross : Select option                │
│  ○ Circle: Exit application             │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│        WEB PAGE VIEW                    │
├─────────────────────────────────────────┤
│  ○ Circle: Return to menu               │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│        FILE BROWSER                     │
├─────────────────────────────────────────┤
│  △ Up    : Previous file                │
│  ▽ Down  : Next file                    │
│  ✕ Cross : Download selected file       │
│  ○ Circle: Return to menu               │
└─────────────────────────────────────────┘
```

## Server URLs

| Endpoint | Purpose | Returns |
|----------|---------|---------|
| http://softa.site/psp | Web page view | HTML content |
| http://softa.site/pspfiles | File list | Text (one file per line) |
| http://softa.site/pspfiles/[file] | Download | File content |

## File Locations

| Location | Contents |
|----------|----------|
| ms0:/PSP/GAME/PSPWebFile/ | Application EBOOT.PBP |
| ms0:/PSP/GAME/ | Downloaded files |

## Build Commands

```bash
make clean    # Clean build directory
make          # Build application
```

## Common Issues

| Problem | Solution |
|---------|----------|
| Network init failed | Configure WiFi in PSP Settings |
| Connection failed | Check WiFi password, signal |
| Can't load page | Verify server is accessible |
| Can't save file | Check memory stick space |

## Quick Setup

### PSP
1. Settings → Network Settings → Connection 1
2. Copy EBOOT.PBP to ms0:/PSP/GAME/PSPWebFile/
3. Launch from Game menu

### Server
1. Place psp.html at /psp
2. Place pspfiles.php at /pspfiles  
3. Create files/ directory
4. Add .htaccess for URL rewriting

## Development

```c
// Key includes
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psputility.h>

// Network init
init_network()
connect_to_ap()

// HTTP request
http_get(url, &len)
extract_http_body(response)
```

## Limits

- Max files: 100
- Visible files: 20 (scrollable)
- Web page lines: 25
- Download location: ms0:/PSP/GAME/
- Network: Connection 1 only
- Protocol: HTTP only (no HTTPS)
