# PSP Web File Browser - Server Files

This directory contains the server-side files needed for the PSP Web File Browser application.

## Files

### psp.html
The web page displayed when accessing http://softa.site/psp from the PSP application.

### pspfiles.php
PHP script that returns a list of downloadable files. Should be accessible at http://softa.site/pspfiles

### files/
Directory containing files available for download. Place any files you want to make available to PSP users here.

## Installation

1. Copy these files to your web server at softa.site
2. Create a `files/` directory next to pspfiles.php
3. Add files you want to make available for download to the files/ directory
4. Ensure the PHP script has read access to the files directory

## File Structure on Server

```
/var/www/softa.site/
├── psp (or psp.html with rewrite)
├── pspfiles (PHP script)
└── files/
    ├── game1.pbp
    ├── theme1.zip
    └── etc...
```

## PHP Configuration

Make sure your server can handle the pspfiles endpoint. You can either:

1. Use .htaccess rewrite rules to make pspfiles.php accessible as /pspfiles
2. Or configure your web server to serve pspfiles.php at the /pspfiles path

Example .htaccess:
```
RewriteEngine On
RewriteRule ^pspfiles$ pspfiles.php [L]
RewriteRule ^psp$ psp.html [L]
```

## File Download

When a user selects a file from the PSP, the application will request:
```
http://softa.site/pspfiles/[filename]
```

Make sure your server serves files from the files/ directory at this path.

You can add this to your PHP script or configure your web server appropriately.
