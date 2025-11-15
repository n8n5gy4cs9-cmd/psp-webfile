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
    MENU_FILELIST
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
            strncpy(hostname, start, hostlen);
            hostname[hostlen] = '\0';
            strcpy(path, slash);
        } else {
            strcpy(hostname, start);
            strcpy(path, "/");
        }
    } else {
        return NULL;
    }
    
    /* Create socket */
    sock = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;
    
    /* Resolve hostname */
    struct in_addr ip;
    if (sceNetInetInetAton(hostname, &ip) == 0) {
        /* Need DNS resolution */
        int rid;
        char buf[1024];
        if (sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0) {
            sceNetInetClose(sock);
            return NULL;
        }
        
        if (sceNetResolverStartNtoA(rid, hostname, &ip, 2, 3) < 0) {
            sceNetResolverDelete(rid);
            sceNetInetClose(sock);
            return NULL;
        }
        sceNetResolverDelete(rid);
    }
    
    /* Connect */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = ip;
    
    if (sceNetInetConnect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        sceNetInetClose(sock);
        return NULL;
    }
    
    /* Send request */
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, hostname);
    
    sceNetInetSend(sock, request, strlen(request), 0);
    
    /* Receive response */
    response = (char*)malloc(response_capacity);
    if (!response) {
        sceNetInetClose(sock);
        return NULL;
    }
    
    while (1) {
        char buf[1024];
        int n = sceNetInetRecv(sock, buf, sizeof(buf), 0);
        
        if (n <= 0) break;
        
        if (response_size + n >= response_capacity) {
            response_capacity *= 2;
            char* new_response = (char*)realloc(response, response_capacity);
            if (!new_response) {
                free(response);
                sceNetInetClose(sock);
                return NULL;
            }
            response = new_response;
        }
        
        memcpy(response + response_size, buf, n);
        response_size += n;
    }
    
    sceNetInetClose(sock);
    
    if (response_size > 0) {
        response[response_size] = '\0';
        if (out_len) *out_len = response_size;
        return response;
    }
    
    free(response);
    return NULL;
}

/* Extract body from HTTP response */
static char* extract_http_body(const char* response)
{
    const char* body = strstr(response, "\r\n\r\n");
    if (body) {
        body += 4;
        return strdup(body);
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
    
    printf("  Loading http://softa.site/psp ...\n\n");
    
    int len;
    char* response = http_get("http://softa.site/psp", &len);
    
    if (response) {
        char* body = extract_http_body(response);
        if (body) {
            /* Display first 30 lines of content */
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
        }
        free(response);
        
        printf("\n  Press Circle to return to menu\n");
    } else {
        printf("  Error: Could not load page\n");
        printf("  Check your network connection\n\n");
        printf("  Press Circle to return to menu\n");
    }
}

/* Fetch and parse file list */
static void fetch_file_list(void)
{
    file_count = 0;
    
    int len;
    char* response = http_get("http://softa.site/pspfiles", &len);
    
    if (!response) return;
    
    char* body = extract_http_body(response);
    if (!body) {
        free(response);
        return;
    }
    
    /* Parse file list - expect one filename per line */
    char* line = strtok(body, "\n\r");
    while (line && file_count < MAX_FILES) {
        /* Skip empty lines */
        while (*line == ' ' || *line == '\t') line++;
        if (*line != '\0') {
            strncpy(file_list[file_count].filename, line, 255);
            file_list[file_count].filename[255] = '\0';
            file_count++;
        }
        line = strtok(NULL, "\n\r");
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
    
    if (file_count == 0) {
        pspDebugScreenClear();
        pspDebugScreenSetXY(0, 1);
        printf("  No files found or connection error\n\n");
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
            snprintf(url, sizeof(url), "http://softa.site/pspfiles/%s", 
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
                if (menu_selection < 1) {
                    menu_selection++;
                    need_redraw = 1;
                }
            }
            
            if ((pad.Buttons & PSP_CTRL_CROSS) && !(oldpad.Buttons & PSP_CTRL_CROSS)) {
                if (menu_selection == 0) {
                    state = MENU_WEBVIEW;
                } else if (menu_selection == 1) {
                    state = MENU_FILELIST;
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
        }
        
        oldpad = pad;
        sceDisplayWaitVblankStart();
    }
    
    /* Cleanup */
    if (net_initialized) {
        sceNetApctlDisconnect();
        sceNetApctlTerm();
        sceNetInetTerm();
        sceNetTerm();
    }
    
    sceKernelExitGame();
    return 0;
}
