/*
 * PSP Web File Browser
 * View web pages and download files over HTTP
 */

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <string.h>
#include <stdlib.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psputility.h>
#include <psputility_netparam.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>

PSP_MODULE_INFO("PSPWebFile", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

/* Define printf to use pspDebugScreenPrintf */
#define printf pspDebugScreenPrintf

/* Menu states */
typedef enum {
    MENU_MAIN,
    MENU_WEBVIEW,
    MENU_FILELIST,
    MENU_TEXTTEST
} MenuState;

/* Network connection state */
static int net_initialized = 0;

/* File list structure */
#define MAX_FILES 100
typedef struct {
    char filename[256];
} FileEntry;

static FileEntry file_list[MAX_FILES];
static int file_count = 0;
static int selected_file = 0;

/* Helper to append file entry safely */
static int add_file_entry(const char* name)
{
    if (!name || !*name) return 0;
    if (file_count >= MAX_FILES) return 0;
    size_t len = strnlen(name, 255);
    if (len == 0) return 0;
    strncpy(file_list[file_count].filename, name, 255);
    file_list[file_count].filename[255] = '\0';
    file_count++;
    return 1;
}

/* Parse Apache-style directory listing HTML */
static int parse_directory_listing_html(const char* html)
{
    if (!html) return 0;
    int added = 0;
    const char* p = html;
    while ((p = strstr(p, "<a")) != NULL && file_count < MAX_FILES) {
        const char* href = strstr(p, "href=\"");
        if (!href) { p += 2; continue; }
        href += 6; /* skip href=" */
        const char* end = strchr(href, '\"');
        if (!end) break;
        size_t len = (size_t)(end - href);
        if (len == 0 || len >= 255) { p = end; continue; }
        char name[256];
        memcpy(name, href, len);
        name[len] = '\0';

        /* Skip parent dirs, subdirectories, query links */
        if (strcmp(name, "../") == 0 || name[0] == '?' || name[0] == '/') {
            p = end;
            continue;
        }
        size_t namelen = strlen(name);
        if (namelen == 0 || name[namelen - 1] == '/') { p = end; continue; }
        if (strstr(name, "/")) { p = end; continue; }

        if (add_file_entry(name)) {
            added++;
        }
        p = end;
    }
    return added;
}

/* ========================= Network Functions ========================= */

/* Initialize network */
static int init_network(void)
{
    int err;
    
    if (net_initialized) return 0;
    
    /* Load network modules */
    err = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    if (err < 0) return err;
    
    err = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    if (err < 0) return err;
    
    /* Initialize network */
    err = sceNetInit(128*1024, 42, 0, 42, 0);
    if (err < 0) return err;
    
    err = sceNetInetInit();
    if (err < 0) return err;
    
    err = sceNetApctlInit(0x10000, 48);
    if (err < 0) return err;
    
    /* Initialize DNS resolver */
    err = sceNetResolverInit();
    if (err < 0) return err;
    
    net_initialized = 1;
    return 0;
}

/* Connect to access point */
static int connect_to_ap(void)
{
    int err;
    int state = 0;
    
    /* Get first connection config */
    err = sceNetApctlConnect(1);
    if (err < 0) {
        return err;
    }
    
    /* Wait for connection */
    while (1) {
        err = sceNetApctlGetState(&state);
        if (err < 0) return err;
        
        if (state == PSP_NET_APCTL_STATE_GOT_IP) {
            break;
        }
        
        sceKernelDelayThread(50000); /* 50ms */
    }
    
    return 0;
}

/* Simple HTTP GET request */
static char* http_get(const char* url, int* out_len)
{
    char hostname[256];
    char path[512];
    int port = 80;
    int sock;
    struct sockaddr_in addr;
    char request[1024];
    char* response = NULL;
    int response_size = 0;
    int response_capacity = 4096;
    
    /* Parse URL - assume http://hostname/path format */
    if (strncmp(url, "http://", 7) == 0) {
        const char* start = url + 7;
        const char* slash = strchr(start, '/');
        
        if (slash) {
            int hostlen = slash - start;
            if (hostlen >= 256) hostlen = 255;
            strncpy(hostname, start, hostlen);
            hostname[hostlen] = '\0';
            strncpy(path, slash, 511);
            path[511] = '\0';
        } else {
            strncpy(hostname, start, 255);
            hostname[255] = '\0';
            strcpy(path, "/");
        }
    } else {
        if (out_len) *out_len = -1;
        return NULL;
    }
    
    /* Create socket */
    sock = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        if (out_len) *out_len = -2;
        return NULL;
    }
    
    /* Resolve hostname */
    struct in_addr ip;
    memset(&ip, 0, sizeof(ip));
    
    if (sceNetInetInetAton(hostname, &ip) == 0) {
        /* Need DNS resolution */
        int rid;
        char resolver_buf[1024];
        int err;
        
        err = sceNetResolverCreate(&rid, resolver_buf, sizeof(resolver_buf));
        if (err < 0) {
            if (out_len) *out_len = -3;
            sceNetInetClose(sock);
            return NULL;
        }
        
        /* Try DNS resolution with longer timeout */
        err = sceNetResolverStartNtoA(rid, hostname, &ip, 5, 5);
        if (err < 0) {
            /* DNS failed - try to continue anyway with error */
            if (out_len) *out_len = -4;
            sceNetResolverDelete(rid);
            sceNetInetClose(sock);
            return NULL;
        }
        
        /* Wait a bit for resolution to complete */
        sceKernelDelayThread(100000); /* 100ms */
        
        sceNetResolverDelete(rid);
    }
    
    /* Verify we got a valid IP */
    if (ip.s_addr == 0) {
        if (out_len) *out_len = -4;
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Connect */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = ip;
    
    /* Set socket to non-blocking for timeout control */
    int nbio = 1;
    sceNetInetSetsockopt(sock, 0xFFFF, 0x1200, &nbio, sizeof(nbio)); /* SO_NBIO */
    
    int conn_result = sceNetInetConnect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (conn_result < 0 && conn_result != 0x80410118) { /* 0x80410118 = in progress */
        if (out_len) *out_len = -5;
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Wait for connection with select */
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);
    struct SceNetInetTimeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    
    if (sceNetInetSelect(sock + 1, NULL, &writefds, NULL, &tv) <= 0) {
        if (out_len) *out_len = -5;
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Set back to blocking */
    nbio = 0;
    sceNetInetSetsockopt(sock, 0xFFFF, 0x1200, &nbio, sizeof(nbio)); /* SO_NBIO */
    
    /* Send request */
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: PSP\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, hostname);
    
    int sent = sceNetInetSend(sock, request, strlen(request), 0);
    if (sent < 0) {
        if (out_len) *out_len = -7;
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Receive response */
    response = (char*)malloc(response_capacity);
    if (!response) {
        if (out_len) *out_len = -8;
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Set receive timeout */
    struct SceNetInetTimeval recv_tv;
    recv_tv.tv_sec = 10;
    recv_tv.tv_usec = 0;
    sceNetInetSetsockopt(sock, 0xFFFF, 0x1006, &recv_tv, sizeof(recv_tv)); /* SO_RCVTIMEO */
    
    int recv_count = 0;
    int max_loops = 100; /* Prevent infinite loop */
    
    while (recv_count < max_loops) {
        char buf[512];
        int n = sceNetInetRecv(sock, buf, sizeof(buf) - 1, 0);
        
        if (n < 0) {
            int err = sceNetInetGetErrno();
            /* EAGAIN or EWOULDBLOCK means no more data */
            if (err == 11 || err == 35) {
                if (response_size > 0) break;
            }
            /* Actual error */
            if (response_size == 0) {
                if (out_len) *out_len = -9;
                free(response);
                sceNetInetClose(sock);
                return NULL;
            }
            break;
        }
        
        if (n == 0) break; /* Connection closed */
        
        recv_count++;
        
        if (response_size + n >= response_capacity) {
            response_capacity *= 2;
            char* new_response = (char*)realloc(response, response_capacity);
            if (!new_response) {
                free(response);
                sceNetInetClose(sock);
                if (out_len) *out_len = -10;
                return NULL;
            }
            response = new_response;
        }
        
        memcpy(response + response_size, buf, n);
        response_size += n;
        
        /* Check if we got a complete response */
        if (response_size > 4) {
            response[response_size] = '\0';
            /* If we see end of headers and some content, we might be done */
            if (strstr(response, "\r\n\r\n") || strstr(response, "\n\n")) {
                /* Short response - we're probably done */
                if (response_size < 4096) {
                    /* Wait a tiny bit for any remaining data */
                    sceKernelDelayThread(50000); /* 50ms */
                }
            }
        }
    }
    
    sceNetInetClose(sock);
    
    if (response_size > 0) {
        response[response_size] = '\0';
        if (out_len) *out_len = response_size;
        return response;
    }
    
    if (out_len) *out_len = -6; /* Empty response */
    free(response);
    return NULL;
}

/* Extract body from HTTP response */
static char* extract_http_body(const char* response)
{
    if (!response) return NULL;
    
    /* Try standard HTTP header separator */
    const char* body = strstr(response, "\r\n\r\n");
    if (body) {
        body += 4;
        return strdup(body);
    }
    
    /* Try Unix-style line endings */
    body = strstr(response, "\n\n");
    if (body) {
        body += 2;
        return strdup(body);
    }
    
    /* If no headers found, maybe it's just plain text - return as-is */
    if (strstr(response, "HTTP/") == NULL) {
        return strdup(response);
    }
    
    return NULL;
}

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();

    return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
    int thid = 0;

    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0)
    {
        sceKernelStartThread(thid, 0, 0);
    }

    return thid;
}

/* Display main menu */
static void draw_main_menu(int selected)
{
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 2);
    
    printf("  =====================================\n");
    printf("       PSP WEB FILE BROWSER\n");
    printf("  =====================================\n\n");
    
    if (selected == 0) {
        printf("  > 1. View Web Page\n");
    } else {
        printf("    1. View Web Page\n");
    }
    
    if (selected == 1) {
        printf("  > 2. Browse & Download Files\n");
    } else {
        printf("    2. Browse & Download Files\n");
    }

    if (selected == 2) {
        printf("  > 3. Simple Text Test\n");
    } else {
        printf("    3. Simple Text Test\n");
    }
    
    printf("\n\n");
    printf("  D-Pad Up/Down: Navigate\n");
    printf("  X: Select\n");
    printf("  Circle: Exit\n");
    printf("\n");
    
    if (net_initialized) {
        printf("  Network: Connected\n");
    } else {
        printf("  Network: Not connected\n");
    }
}

/* Display web page view */
static void view_webpage(void)
{
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 1);
    
    printf("  Loading http://softa.site/psp/ ...\n\n");
    
    int len = 0;
    char* response = http_get("http://softa.site/psp/", &len);
    
    if (response) {
        char* body = extract_http_body(response);
        if (body) {
            /* Display first 25 lines of content */
            printf("  Page Content:\n");
            printf("  =====================================\n");
            
            int lines = 0;
            char* line = strtok(body, "\n");
            while (line && lines < 25) {
                printf("  %s\n", line);
                line = strtok(NULL, "\n");
                lines++;
            }
            
            free(body);
        } else {
            printf("  Error: Could not parse response\n");
            printf("  Received %d bytes\n\n", len);
            /* Show raw response for debugging */
            if (len > 0 && len < 500) {
                printf("  Raw: %s\n\n", response);
            }
        }
        free(response);
        
        printf("\n  Press Circle to return to menu\n");
    } else {
        printf("  Error: Could not load page\n\n");
        if (len == -1) printf("  Invalid URL format\n");
        else if (len == -2) printf("  Socket creation failed\n");
        else if (len == -3) printf("  DNS resolver init failed\n");
        else if (len == -4) {
            printf("  DNS lookup failed\n");
            printf("  Cannot resolve: softa.site\n");
            printf("\n  Solutions:\n");
            printf("  - Check DNS in Network Settings\n");
            printf("  - Try Test Connection\n");
            printf("  - Verify internet works\n");
        }
        else if (len == -5) printf("  Connection refused\n");
        else if (len == -6) printf("  Empty response from server\n");
        else if (len == -7) printf("  Failed to send request\n");
        else if (len == -8) printf("  Memory allocation failed\n");
        else if (len == -9) printf("  Error receiving data\n");
        else if (len == -10) printf("  Out of memory\n");
        else if (len == 0) {
            printf("  Request failed\n");
            printf("  - Check WiFi connection\n");
            printf("  - Verify server is online\n");
            printf("  - Test: http://softa.site/psp/\n");
        }
        else printf("  Unknown (code %d)\n", len);
        printf("\n  Press Circle to return to menu\n");
    }
}

/* Simple connectivity test fetching plain text file */
static void simple_text_test(void)
{
    const char* url = "http://softa.site/pspfiles/files/test.txt";
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 1);
    printf("  Fetching %s...\n\n", url);

    int len = 0;
    char* response = http_get(url, &len);

    if (response) {
        char* body = extract_http_body(response);
        if (body) {
            int is_empty = (body[0] == '\0');
            printf("  test.txt contents:\n");
            printf("  =====================================\n");
            int lines = 0;
            char* line = strtok(body, "\n");
            while (line && lines < 20) {
                printf("  %s\n", line);
                line = strtok(NULL, "\n");
                lines++;
            }
            if (is_empty) {
                printf("  (File is empty)\n");
            }
            printf("\n  Total bytes: %d\n", len);
            free(body);
        } else {
            printf("  Error: Could not parse server response.\n");
            printf("  Received %d bytes.\n", len);
        }
        free(response);
        printf("\n  Press Circle to return to menu\n");
    } else {
        printf("  Error: Could not fetch test.txt\n\n");
        if (len == -1) printf("  Invalid URL format\n");
        else if (len == -2) printf("  Socket creation failed\n");
        else if (len == -3) printf("  DNS resolver init failed\n");
        else if (len == -4) {
            printf("  DNS lookup failed (softa.site)\n");
            printf("  Re-run Test Connection in PSP settings.\n");
        }
        else if (len == -5) printf("  Connection refused\n");
        else if (len == -6) printf("  Empty response\n");
        else if (len == -7) printf("  Failed to send request\n");
        else if (len == -8) printf("  Out of memory\n");
        else if (len == -9) printf("  Error receiving data\n");
        else if (len == -10) printf("  Buffer allocation failed\n");
        else printf("  Unknown error code: %d\n", len);
        printf("\n  Press Circle to return to menu\n");
    }
}

/* Fetch and parse file list */
static void fetch_file_list(void)
{
    file_count = 0;
    
    int len = 0;
    char* response = http_get("http://softa.site/pspfiles/files/", &len);
    
    if (!response) {
        /* Store error code in first file entry for display */
        if (len < 0) {
            snprintf(file_list[0].filename, 255, "[ERROR %d: See docs]", len);
            file_count = 1;
        }
        return;
    }
    
    char* body = extract_http_body(response);
    if (!body) {
        /* Try using response directly if body extraction failed */
        body = strdup(response);
        if (!body) {
            free(response);
            return;
        }
    }
    
    int added = 0;
    if (body && (strstr(body, "<html") || strstr(body, "<HTML") || strstr(body, "<title"))) {
        added = parse_directory_listing_html(body);
    }
    
    if (added == 0) {
        /* Parse plain text list - expect one filename per line */
        char* line = strtok(body, "\n");
        while (line && file_count < MAX_FILES) {
            /* Remove carriage return if present */
            char* cr = strchr(line, '\r');
            if (cr) *cr = '\0';
            
            /* Skip empty lines and whitespace */
            while (*line == ' ' || *line == '\t') line++;
            
            /* Skip error messages and empty lines */
            if (*line != '\0' && strstr(line, "Error:") == NULL) {
                add_file_entry(line);
            }
            line = strtok(NULL, "\n");
        }
    }
    
    free(body);
    free(response);
}

/* Display file list browser */
static void browse_files(void)
{
    int scroll_offset = 0;
    int need_refresh = 1;
    
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 1);
    printf("  Fetching file list...\n");
    
    fetch_file_list();
    
    if (file_count == 0 || (file_count == 1 && file_list[0].filename[0] == '[')) {
        pspDebugScreenClear();
        pspDebugScreenSetXY(0, 1);
        printf("  File Browser Error\n");
        printf("  =====================================\n\n");
        if (file_count == 1) {
            printf("  %s\n\n", file_list[0].filename);
        } else {
            printf("  No files found\n\n");
        }
        printf("  Troubleshooting:\n");
        printf("  1. Test in web browser:\n");
        printf("     http://softa.site/pspfiles/files/\n");
        printf("     (should show file list)\n\n");
        printf("  2. Verify server structure:\n");
        printf("     /pspfiles/pspfiles.php\n");
        printf("     /pspfiles/files/*.mp3\n\n");
        printf("  3. Check permissions: 755\n\n");
        printf("  4. PHP should return:\n");
        printf("     filename1.mp3\n");
        printf("     filename2.mp3\n");
        printf("     (one per line, plain text)\n\n");
        printf("  Press Circle to return to menu\n");
        
        SceCtrlData pad;
        while (1) {
            sceCtrlReadBufferPositive(&pad, 1);
            if (pad.Buttons & PSP_CTRL_CIRCLE) break;
            sceDisplayWaitVblankStart();
        }
        return;
    }
    
    SceCtrlData pad;
    SceCtrlData oldpad;
    memset(&oldpad, 0, sizeof(oldpad));
    
    while (1) {
        if (need_refresh) {
            pspDebugScreenClear();
            pspDebugScreenSetXY(0, 1);
            
            printf("  File Browser (%d files)\n", file_count);
            printf("  =====================================\n\n");
            
            /* Display 20 files at a time */
            int max_display = 20;
            if (selected_file < scroll_offset) {
                scroll_offset = selected_file;
            }
            if (selected_file >= scroll_offset + max_display) {
                scroll_offset = selected_file - max_display + 1;
            }
            
            for (int i = scroll_offset; i < scroll_offset + max_display && i < file_count; i++) {
                if (i == selected_file) {
                    printf("  > %s\n", file_list[i].filename);
                } else {
                    printf("    %s\n", file_list[i].filename);
                }
            }
            
            printf("\n  =====================================\n");
            printf("  Up/Down: Navigate | X: Download\n");
            printf("  Circle: Back to menu\n");
            
            need_refresh = 0;
        }
        
        sceCtrlReadBufferPositive(&pad, 1);
        
        if ((pad.Buttons & PSP_CTRL_UP) && !(oldpad.Buttons & PSP_CTRL_UP)) {
            if (selected_file > 0) {
                selected_file--;
                need_refresh = 1;
            }
        }
        
        if ((pad.Buttons & PSP_CTRL_DOWN) && !(oldpad.Buttons & PSP_CTRL_DOWN)) {
            if (selected_file < file_count - 1) {
                selected_file++;
                need_refresh = 1;
            }
        }
        
        if ((pad.Buttons & PSP_CTRL_CROSS) && !(oldpad.Buttons & PSP_CTRL_CROSS)) {
            /* Download selected file */
            pspDebugScreenSetXY(0, 28);
            printf("  Downloading %s ...", file_list[selected_file].filename);
            
            char url[512];
            snprintf(url, sizeof(url), "http://softa.site/pspfiles/files/%s", 
                     file_list[selected_file].filename);
            
            int len;
            char* data = http_get(url, &len);
            
            if (data) {
                char* body = extract_http_body(data);
                if (body) {
                    /* Save to ms0:/PSP/GAME/ */
                    char filepath[512];
                    snprintf(filepath, sizeof(filepath), "ms0:/PSP/GAME/%s",
                             file_list[selected_file].filename);
                    
                    SceUID fd = sceIoOpen(filepath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
                    if (fd >= 0) {
                        sceIoWrite(fd, body, strlen(body));
                        sceIoClose(fd);
                        pspDebugScreenSetXY(0, 29);
                        printf("  Downloaded to %s", filepath);
                    } else {
                        pspDebugScreenSetXY(0, 29);
                        printf("  Error: Could not save file");
                    }
                    free(body);
                } else {
                    pspDebugScreenSetXY(0, 29);
                    printf("  Error: Invalid response");
                }
                free(data);
            } else {
                pspDebugScreenSetXY(0, 29);
                printf("  Error: Download failed");
            }
            
            sceKernelDelayThread(2000000); /* Wait 2 seconds */
            need_refresh = 1;
        }
        
        if (pad.Buttons & PSP_CTRL_CIRCLE) {
            break;
        }
        
        oldpad = pad;
        sceDisplayWaitVblankStart();
    }
}

int main(void)
{
    SceCtrlData pad;
    SceCtrlData oldpad;
    memset(&oldpad, 0, sizeof(oldpad));
    
    MenuState state = MENU_MAIN;
    int menu_selection = 0;
    int need_redraw = 1;
    
    /* Set up callbacks */
    SetupCallbacks();
    
    /* Initialize the debug screen */
    pspDebugScreenInit();
    pspDebugScreenClear();
    
    /* Initialize network */
    pspDebugScreenSetXY(0, 10);
    printf("  Initializing network...\n");
    
    if (init_network() == 0) {
        printf("  Connecting to access point...\n");
        if (connect_to_ap() == 0) {
            printf("  Connected!\n");
            sceKernelDelayThread(1000000); /* Wait 1 second */
        } else {
            printf("  Connection failed!\n");
            printf("  Some features may not work.\n");
            sceKernelDelayThread(2000000); /* Wait 2 seconds */
        }
    } else {
        printf("  Network initialization failed!\n");
        printf("  Some features may not work.\n");
        sceKernelDelayThread(2000000); /* Wait 2 seconds */
    }
    
    /* Main loop */
    while (1) {
        if (state == MENU_MAIN) {
            if (need_redraw) {
                draw_main_menu(menu_selection);
                need_redraw = 0;
            }
            
            sceCtrlReadBufferPositive(&pad, 1);
            
            if ((pad.Buttons & PSP_CTRL_UP) && !(oldpad.Buttons & PSP_CTRL_UP)) {
                if (menu_selection > 0) {
                    menu_selection--;
                    need_redraw = 1;
                }
            }
            
            if ((pad.Buttons & PSP_CTRL_DOWN) && !(oldpad.Buttons & PSP_CTRL_DOWN)) {
                if (menu_selection < 2) {
                    menu_selection++;
                    need_redraw = 1;
                }
            }
            
            if ((pad.Buttons & PSP_CTRL_CROSS) && !(oldpad.Buttons & PSP_CTRL_CROSS)) {
                if (menu_selection == 0) {
                    state = MENU_WEBVIEW;
                } else if (menu_selection == 1) {
                    state = MENU_FILELIST;
                } else if (menu_selection == 2) {
                    state = MENU_TEXTTEST;
                }
            }
            
            if (pad.Buttons & PSP_CTRL_CIRCLE) {
                break;
            }
        } else if (state == MENU_WEBVIEW) {
            view_webpage();
            
            /* Wait for Circle to return */
            while (1) {
                sceCtrlReadBufferPositive(&pad, 1);
                if (pad.Buttons & PSP_CTRL_CIRCLE) break;
                sceDisplayWaitVblankStart();
            }
            
            state = MENU_MAIN;
            need_redraw = 1;
        } else if (state == MENU_FILELIST) {
            browse_files();
            state = MENU_MAIN;
            need_redraw = 1;
        } else if (state == MENU_TEXTTEST) {
            simple_text_test();
            while (1) {
                sceCtrlReadBufferPositive(&pad, 1);
                if (pad.Buttons & PSP_CTRL_CIRCLE) break;
                sceDisplayWaitVblankStart();
            }
            state = MENU_MAIN;
            need_redraw = 1;
        }
        
        oldpad = pad;
        sceDisplayWaitVblankStart();
    }
    
    /* Cleanup */
    if (net_initialized) {
        sceNetResolverTerm();
        sceNetApctlDisconnect();
        sceNetApctlTerm();
        sceNetInetTerm();
        sceNetTerm();
    }
    
    sceKernelExitGame();
    return 0;
}
