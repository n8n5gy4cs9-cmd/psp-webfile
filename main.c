/*
 * PSP Hello World Application
 * Displays "Hello World" on the PSP screen
 */

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <string.h>
#include <stdlib.h>
#include <psppower.h>
#include <pspiofilemgr.h>

PSP_MODULE_INFO("HelloWorld", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* Define printf to use pspDebugScreenPrintf */
#define printf pspDebugScreenPrintf

/* ========================= Image + Base64 helpers ========================= */

/* Helper for signed area (orientation) - defined at file scope for C89 compatibility */
static int tri_sign(int ax,int ay,int bx,int by,int cx,int cy) {
    return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy);
}

/* Minimal Base64 encode/decode */
static const char B64_TBL[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *b64_encode(const unsigned char *data, size_t len, size_t *out_len)
{
    size_t olen = 4 * ((len + 2) / 3);
    char *out = (char*)malloc(olen + 1);
    if (!out) return NULL;
    size_t i = 0, j = 0;
    while (i < len) {
        unsigned int v = data[i++] << 16;
        if (i < len) v |= data[i++] << 8;
        if (i < len) v |= data[i++];
        out[j++] = B64_TBL[(v >> 18) & 0x3F];
        out[j++] = B64_TBL[(v >> 12) & 0x3F];
        out[j++] = (i > (len + 1)) ? '=' : B64_TBL[(v >> 6) & 0x3F];
        out[j++] = (i > len) ? '=' : B64_TBL[v & 0x3F];
    }
    out[j] = '\0';
    if (out_len) *out_len = j;
    return out;
}

static int b64_idx(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static unsigned char* b64_decode(const char* in, size_t inlen, size_t* outlen) {
    unsigned char* out = (unsigned char*)malloc((inlen / 4 + 1) * 3);
    if (!out) return NULL;

    size_t o = 0;
    int val = 0, valb = -8;
    for (size_t i = 0; i < inlen; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '=') break;
        int d = b64_idx((char)c);
        if (d < 0) continue; /* skip whitespace/invalid */
        val = (val << 6) | d;
        valb += 6;
        if (valb >= 0) {
            out[o++] = (unsigned char)((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    if (outlen) *outlen = o;
    return out;
}

/* Draw an RGBA8888 image to current framebuffer, converting to destination format if needed */
static void blit_rgba8888_to_screen(int dstX, int dstY, int width, int height, const unsigned char* rgba, size_t rgba_len)
{
    if (!rgba || rgba_len < (size_t)(width * height * 4)) return;

    void* topaddr = NULL;
    int bufferwidth = 0;
    int pixelformat = 0;

    if (sceDisplayGetFrameBuf(&topaddr, &bufferwidth, &pixelformat, PSP_DISPLAY_SETBUF_IMMEDIATE) < 0) return;
    if (!topaddr || bufferwidth <= 0) return;

    if (pixelformat == PSP_DISPLAY_PIXEL_FORMAT_565) {
        volatile unsigned short* vram = (volatile unsigned short*)topaddr;
        for (int y = 0; y < height; y++) {
            int sy = dstY + y; if (sy < 0 || sy >= 272) continue;
            volatile unsigned short* row = vram + (sy * bufferwidth);
            for (int x = 0; x < width; x++) {
                int sx = dstX + x; if (sx < 0 || sx >= 480) continue;
                const unsigned char* p = rgba + (y * width + x) * 4;
                unsigned char r = p[0], g = p[1], b = p[2], a = p[3];
                if (a == 0) continue; /* respect transparency */
                /* Compose standard RGB565; VRAM is little-endian but writing as 16-bit handles endianness */
                unsigned short rgb565 = (unsigned short)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
                row[sx] = rgb565;
            }
        }
    } else {
        /* Default to 8888 (0xAARRGGBB) */
        volatile unsigned int* vram = (volatile unsigned int*)topaddr;
        for (int y = 0; y < height; y++) {
            int sy = dstY + y; if (sy < 0 || sy >= 272) continue;
            volatile unsigned int* row = vram + (sy * bufferwidth);
            for (int x = 0; x < width; x++) {
                int sx = dstX + x; if (sx < 0 || sx >= 480) continue;
                const unsigned char* p = rgba + (y * width + x) * 4;
                unsigned char r = p[0], g = p[1], b = p[2], a = p[3];
                if (a == 0) continue;
                /* Many setups expect 0xAABBGGRR in VRAM due to little-endian; swap R/B here */
                unsigned int abgr = ((unsigned int)a << 24) | ((unsigned int)b << 16) | ((unsigned int)g << 8) | (unsigned int)r;
                row[sx] = abgr;
            }
        }
    }
}

/* Generate a simple yellow warning triangle with black border and exclamation mark into RGBA8888 buffer */
static unsigned char* gen_warning_icon_rgba(int w, int h)
{
    if (w <= 0 || h <= 0) return NULL;
    size_t sz = (size_t)w * h * 4;
    unsigned char* buf = (unsigned char*)malloc(sz);
    if (!buf) return NULL;
    memset(buf, 0, sz);

    /* Triangle vertices */
    int x0 = w / 2, y0 = 1;       /* top */
    int x1 = 1,     y1 = h - 2;   /* bottom-left */
    int x2 = w - 2, y2 = h - 2;   /* bottom-right */

    /* Fill triangle */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int p = (y * w + x) * 4;
            int s1 = tri_sign(x, y, x1, y1, x0, y0);
            int s2 = tri_sign(x, y, x2, y2, x1, y1);
            int s3 = tri_sign(x, y, x0, y0, x2, y2);
            int has_neg = (s1 < 0) || (s2 < 0) || (s3 < 0);
            int has_pos = (s1 > 0) || (s2 > 0) || (s3 > 0);
            int inside = !(has_neg && has_pos);
            if (inside) {
                /* Check border by testing 4-neighborhood */
                int border = 0;
                int nx[4] = {x-1, x+1, x, x};
                int ny[4] = {y, y, y-1, y+1};
                for (int k=0;k<4;k++) {
                    int xx = nx[k], yy = ny[k];
                    if (xx < 0 || yy < 0 || xx >= w || yy >= h) { border = 1; break; }
                    int ss1 = tri_sign(xx, yy, x1, y1, x0, y0);
                    int ss2 = tri_sign(xx, yy, x2, y2, x1, y1);
                    int ss3 = tri_sign(xx, yy, x0, y0, x2, y2);
                    int n_has_neg = (ss1 < 0) || (ss2 < 0) || (ss3 < 0);
                    int n_has_pos = (ss1 > 0) || (ss2 > 0) || (ss3 > 0);
                    int n_inside = !(n_has_neg && n_has_pos);
                    if (!n_inside) { border = 1; break; }
                }
                if (border) {
                    /* black border */
                    buf[p+0] = 0; buf[p+1] = 0; buf[p+2] = 0; buf[p+3] = 255;
                } else {
                    /* yellow fill */
                    buf[p+0] = 255; buf[p+1] = 208; buf[p+2] = 0; buf[p+3] = 255;
                }
            }
        }
    }

    /* Exclamation mark */
    int cx = w/2;
    int lineTop = (int)(h * 0.35f);
    int lineBot = (int)(h * 0.68f);
    for (int y = lineTop; y <= lineBot; y++) {
        for (int dx = -1; dx <= 1; dx++) {
            int x = cx + dx; if (x < 0 || x >= w || y < 0 || y >= h) continue;
            int p = (y * w + x) * 4;
            buf[p+0] = 0; buf[p+1] = 0; buf[p+2] = 0; buf[p+3] = 255;
        }
    }
    /* Dot */
    int dy = (int)(h * 0.80f);
    for (int y = dy; y < dy + 2 && y < h; y++) {
        for (int x = cx-1; x <= cx; x++) {
            if (x < 0 || x >= w) continue;
            int p = (y * w + x) * 4;
            buf[p+0] = 0; buf[p+1] = 0; buf[p+2] = 0; buf[p+3] = 255;
        }
    }

    return buf;
}

/* Convenience: generate -> base64 -> decode -> blit */
static void draw_warning_icon_b64(int x, int y, int w, int h)
{
    unsigned char *rgba = gen_warning_icon_rgba(w, h);
    if (!rgba) return;
    size_t img_sz = (size_t)w * h * 4;
    size_t b64_len = 0;
    char *b64 = b64_encode(rgba, img_sz, &b64_len);
    if (!b64) { free(rgba); return; }
    size_t dec_len = 0;
    unsigned char *decoded = b64_decode(b64, b64_len, &dec_len);
    if (decoded && dec_len == img_sz) {
        blit_rgba8888_to_screen(x, y, w, h, decoded, dec_len);
    }
    if (decoded) free(decoded);
    free(b64);
    free(rgba);
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

int main(void)
{
    SceCtrlData pad;
    SceCtrlData oldpad;
    memset(&oldpad, 0, sizeof(oldpad));
    
    /* Background color cycling for dead pixel testing */
    int bg_index = 0;
    unsigned int bg_colors[] = {
        0x00000000,  /* Black (default) */
        0x000000FF,  /* Red */
        0x0000FF00,  /* Green */
        0x00FF0000,  /* Blue */
        0x00FFFF00,  /* Cyan */
        0x00FF00FF,  /* Magenta */
        0x0000FFFF,  /* Yellow */
        0x00FFFFFF   /* White */
    };
    int num_bg_colors = sizeof(bg_colors) / sizeof(bg_colors[0]);
    
    /* Set up callbacks */
    SetupCallbacks();

    /* Initialize the debug screen */
    pspDebugScreenInit();

    /* Debug: report current framebuffer pixel format once */
    {
        void* topaddr = NULL; int bw = 0; int pf = 0;
        if (sceDisplayGetFrameBuf(&topaddr, &bw, &pf, PSP_DISPLAY_SETBUF_IMMEDIATE) >= 0) {
            printf("\n[debug] pixelformat=%d (0=565, 1=5551, 2=4444, 3=8888)\n", pf);
        }
    }

    /* Clear screen and set cursor to top-left */
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);

    /* Display Hello World message */
    printf("\n\n");
    printf("  =====================================\n");
    printf("       PSP CROWELIAN - HELLO WORLD     \n");
    printf("  =====================================\n\n");
    printf("  Hello World!\n\n");
    printf("  Welcome Harris PSP world!\n\n");
    printf("  Press X to exit.\n\n");
    printf("  Press Triangle to cycle background.\n\n");
    printf("  =====================================\n");

    /* --- 3–4 blank lines before specs --- */
    printf("\n\n\n");

    /* PSP specs (user-mode accessible) */
    {
        /* Firmware (devkit) version */
        unsigned int dv = sceKernelDevkitVersion();
        int fw_major = (dv >> 24) & 0xFF;
        int fw_minor = (dv >> 16) & 0xFF;
        int fw_rev   = dv & 0xFFFF;

        /* Display info */
        int mode = 0, dw = 0, dh = 0;
        sceDisplayGetMode(&mode, &dw, &dh);
        void* topaddr2 = NULL; int bw2 = 0; int pf2 = 0;
        sceDisplayGetFrameBuf(&topaddr2, &bw2, &pf2, PSP_DISPLAY_SETBUF_IMMEDIATE);

        /* CPU/bus clocks */
        int cpu_clk = scePowerGetCpuClockFrequencyInt();
        int bus_clk = scePowerGetBusClockFrequencyInt();

        /* Memory (user-mode heap) */
        SceSize max_free = sceKernelMaxFreeMemSize();
        SceSize tot_free = sceKernelTotalFreeMemSize();

        /* Battery status */
        int bat_exist = scePowerIsBatteryExist();
        int on_ac     = scePowerIsPowerOnline();
        int charging  = scePowerIsBatteryCharging();
        int bat_pct   = scePowerGetBatteryLifePercent();

        printf("  PSP Specs:\n");
        printf("   - Firmware (devkit): %d.%d (rev 0x%04X)\n", fw_major, fw_minor, fw_rev);
        printf("   - Display: %dx%d, bufferwidth=%d, pixelfmt=%d \n       (0=565,1=5551,2=4444,3=8888)\n", dw, dh, bw2, pf2);
        printf("   - Clocks: CPU=%d MHz, BUS=%d MHz\n", cpu_clk, bus_clk);
        printf("   - Memory: max free=%lu KB, total free=%lu KB\n",
               (unsigned long)(max_free/1024), (unsigned long)(tot_free/1024));
        if (bat_exist)
            printf("   - Battery: %d%%, AC=%s, Charging=%s\n",
                   bat_pct, on_ac ? "Yes" : "No", charging ? "Yes" : "No");
        else
            printf("   - Battery: Not present (AC/emulator)\n");

        /* Best-effort model guess (user-mode heuristics) */
        {
            int is_go = 0;
            SceUID dfd = sceIoDopen("ef0:/");
            if (dfd >= 0) { is_go = 1; sceIoDclose(dfd); }

            unsigned long free_kb = (unsigned long)(tot_free / 1024);
            int is_ppsspp = 0;
            /* PPSSPP often reports unusually high free memory or 0% battery */
            if (free_kb > 100000UL || (!bat_exist && tot_free > 60*1024*1024)) {
                is_ppsspp = 1;
            }

            if (is_ppsspp) {
                printf("   - Model (guess): PPSSPP Emulator\n");
            } else if (is_go) {
                printf("   - Model (guess): PSP Go (N1000)\n");
            } else if (free_kb > 40000UL) {
                printf("   - Model (guess): 64MB model (PSP-2000/3000/E1000)\n");
            } else {
                printf("   - Model (guess): 32MB model (PSP-1000)\n");
            }
            if (!is_ppsspp) {
                printf("   - Note: Exact 2000 vs 3000 and LCD panel require\n");
                printf("           kernel tools (PSPident) for definitive info.\n");
            }
        }
    }

    /* Draw a yellow warning icon (generated -> base64 -> decoded) */
    draw_warning_icon_b64(240, 108, 24, 24);

    /* Main loop */
    while(1)
    {
        /* Read controller input */
        sceCtrlReadBufferPositive(&pad, 1);

        /* Triangle button: cycle background color for dead pixel testing */
        if ((pad.Buttons & PSP_CTRL_TRIANGLE) && !(oldpad.Buttons & PSP_CTRL_TRIANGLE)) {
            bg_index = (bg_index + 1) % num_bg_colors;
            
            /* Clear screen with new background color */
            pspDebugScreenSetBackColor(bg_colors[bg_index]);
            pspDebugScreenClear();
            pspDebugScreenSetXY(0, 0);
            
            /* Redraw everything */
            printf("\n\n");
            printf("  =====================================\n");
            printf("       PSP CROWELIAN - HELLO WORLD     \n");
            printf("  =====================================\n\n");
            printf("  Hello World!\n\n");
            printf("  Welcome Harris PSP world!\n\n");
            printf("  Press X to exit.\n");
            printf("  Press Triangle to cycle background.\n\n");
            printf("  =====================================\n");
            
            printf("\n\n\n");
            
            /* Redraw specs block */
            {
                unsigned int dv = sceKernelDevkitVersion();
                int fw_major = (dv >> 24) & 0xFF;
                int fw_minor = (dv >> 16) & 0xFF;
                int fw_rev   = dv & 0xFFFF;
                int mode = 0, dw = 0, dh = 0;
                sceDisplayGetMode(&mode, &dw, &dh);
                void* topaddr2 = NULL; int bw2 = 0; int pf2 = 0;
                sceDisplayGetFrameBuf(&topaddr2, &bw2, &pf2, PSP_DISPLAY_SETBUF_IMMEDIATE);
                int cpu_clk = scePowerGetCpuClockFrequencyInt();
                int bus_clk = scePowerGetBusClockFrequencyInt();
                SceSize max_free = sceKernelMaxFreeMemSize();
                SceSize tot_free = sceKernelTotalFreeMemSize();
                int bat_exist = scePowerIsBatteryExist();
                int on_ac = scePowerIsPowerOnline();
                int charging = scePowerIsBatteryCharging();
                int bat_pct = scePowerGetBatteryLifePercent();
                
                printf("  PSP Specs:\n");
                printf("   - Firmware (devkit): %d.%d (rev 0x%04X)\n", fw_major, fw_minor, fw_rev);
                printf("   - Display: %dx%d, bufferwidth=%d, pixelfmt=%d \n       (0=565,1=5551,2=4444,3=8888)\n", dw, dh, bw2, pf2);
                printf("   - Clocks: CPU=%d MHz, BUS=%d MHz\n", cpu_clk, bus_clk);
                printf("   - Memory: max free=%lu KB, total free=%lu KB\n",
                       (unsigned long)(max_free/1024), (unsigned long)(tot_free/1024));
                if (bat_exist)
                    printf("   - Battery: %d%%, AC=%s, Charging=%s\n",
                           bat_pct, on_ac ? "Yes" : "No", charging ? "Yes" : "No");
                else
                    printf("   - Battery: Not present (AC/emulator)\n");
                
                {
                    int is_go = 0;
                    SceUID dfd = sceIoDopen("ef0:/");
                    if (dfd >= 0) { is_go = 1; sceIoDclose(dfd); }
                    unsigned long free_kb = (unsigned long)(tot_free / 1024);
                    int is_ppsspp = 0;
                    if (free_kb > 100000UL || (!bat_exist && tot_free > 60*1024*1024)) {
                        is_ppsspp = 1;
                    }
                    if (is_ppsspp) {
                        printf("   - Model (guess): PPSSPP Emulator\n");
                    } else if (is_go) {
                        printf("   - Model (guess): PSP Go (N1000)\n");
                    } else if (free_kb > 40000UL) {
                        printf("   - Model (guess): 64MB model (PSP-2000/3000/E1000)\n");
                    } else {
                        printf("   - Model (guess): 32MB model (PSP-1000)\n");
                    }
                    if (!is_ppsspp) {
                        printf("   - Note: Exact 2000 vs 3000 and LCD panel require\n");
                        printf("           kernel tools (PSPident) for definitive info.\n");
                    }
                }
            }
            
            /* Redraw warning icon */
            draw_warning_icon_b64(240, 108, 24, 24);
            
            /* Show current background color name */
            const char* color_names[] = {
                "Black", "Red", "Green", "Blue", "Cyan", "Magenta", "Yellow", "White"
            };
            printf("\n  BG Color: %s (%d/%d)\n", color_names[bg_index], bg_index + 1, num_bg_colors);
        }

        /* Check if X button is pressed */
        if(pad.Buttons & PSP_CTRL_CROSS)
        {
            break;
        }

        oldpad = pad;

        /* Wait a bit before next check */
        sceDisplayWaitVblankStart();
    }

    /* Exit */
    sceKernelExitGame();
    return 0;
}
