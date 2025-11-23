<?php
/**
 * PSP File List Server
 * Returns a list of files available for download
 * Place this at http://softa.site/pspfiles
 */

header('Content-Type: text/plain');

// Directory containing files for download
$files_dir = __DIR__ . '/files/';

// Debug: show what path we're checking
// echo "Checking: $files_dir\n";
// echo "Dir exists: " . (is_dir($files_dir) ? 'YES' : 'NO') . "\n";
// echo "---\n";

// Get list of files
if (is_dir($files_dir)) {
    $files = scandir($files_dir);
    
    foreach ($files as $file) {
        // Skip . and ..
        if ($file == '.' || $file == '..') {
            continue;
        }
        
        // Only list files, not directories
        if (is_file($files_dir . $file)) {
            echo $file . "\n";
        }
    }
} else {
    echo "Error: Files directory not found\n";
}
?>
