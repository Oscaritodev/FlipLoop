#include <citro2d.h>
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

// Colors
#define CLR_TIKTOK_BG    C2D_Color32(0, 0, 0, 255)
#define CLR_TIKTOK_TEXT  C2D_Color32(255, 255, 255, 255)
#define CLR_ACCENT_RED   C2D_Color32(254, 44, 85, 255)
#define CLR_ACCENT_CYAN  C2D_Color32(37, 244, 238, 255)
#define CLR_GRAY         C2D_Color32(100, 100, 100, 255)

typedef enum {
    STATE_LOGIN,
    STATE_FEED,
    STATE_VIDEO
} AppState;

AppState g_state = STATE_LOGIN;
char g_sessionToken[64] = {0};
char g_statusMessage[128] = "Press A to pair device";

// Feed Data (Simplified)
#define MAX_VIDEOS 5
char g_videoTitles[MAX_VIDEOS][64];
char g_videoAuthors[MAX_VIDEOS][32];
int g_videoCount = 0;
int g_selectedVideo = 0;

// Graphics
C3D_RenderTarget* top = NULL;
C3D_RenderTarget* bottom = NULL;
C2D_TextBuf g_staticBuf;
C2D_Text g_textObj;

void updateStatus(const char* msg) {
    strncpy(g_statusMessage, msg, sizeof(g_statusMessage)-1);
    C2D_TextParse(&g_textObj, g_staticBuf, g_statusMessage);
    C2D_TextOptimize(&g_textObj);
}

void initGraphics() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    g_staticBuf = C2D_TextBufNew(4096);
    updateStatus("Welcome to 3dsTok");
}

void parseFeed(char* json) {
    // VERY primitive parsing for demo purposes
    // Finds "description":"..." and "author":"..."
    g_videoCount = 0;
    char* cursor = json;
    
    while(g_videoCount < MAX_VIDEOS) {
        char* desc = strstr(cursor, "\"description\":\"");
        if(!desc) break;
        desc += 15; // skip key
        char* descEnd = strchr(desc, '"');
        if(!descEnd) break;
        
        int len = descEnd - desc;
        if(len > 63) len = 63;
        strncpy(g_videoTitles[g_videoCount], desc, len);
        g_videoTitles[g_videoCount][len] = '\0';
        
        char* auth = strstr(descEnd, "\"author\":\"");
        if(auth) {
            auth += 10;
            char* authEnd = strchr(auth, '"');
            if(authEnd) {
                int aLen = authEnd - auth;
                if(aLen > 31) aLen = 31;
                strncpy(g_videoAuthors[g_videoCount], auth, aLen);
                g_videoAuthors[g_videoCount][aLen] = '\0';
            }
        }
        
        cursor = descEnd;
        g_videoCount++;
    }
}

void doLogin() {
    SwkbdState swkbd;
    char myCode[10];
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, 8);
    swkbdSetHintText(&swkbd, "Enter Pairing Code");
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    
    SwkbdButton button = swkbdInputText(&swkbd, myCode, sizeof(myCode));
    
    if (button != SWKBD_BUTTON_CONFIRM) return;
    
    updateStatus("Verifying code...");
    // Render one frame to show status
    // (In real loop this would be async, but blocking is fine here)
    
    // JSON Body
    char body[64];
    snprintf(body, sizeof(body), "{\"code\":\"%s\"}", myCode);
    
    char response[1024];
    Result ret = network_post("/api/verify_code", body, response, sizeof(response));
    
    if (R_FAILED(ret)) {
        updateStatus("Network Error!");
    } else {
        // Check for session_token
        char* tokenPtr = strstr(response, "\"session_token\":\"");
        if (tokenPtr) {
            tokenPtr += 17;
            char* end = strchr(tokenPtr, '"');
            if(end) {
                int len = end - tokenPtr;
                if(len > 63) len = 63;
                strncpy(g_sessionToken, tokenPtr, len);
                g_sessionToken[len] = '\0';
                
                g_state = STATE_FEED;
                updateStatus("Login Success! Loading Feed...");
                
                // Load Feed
                ret = network_get("/api/feed", g_sessionToken, response, sizeof(response));
                if(R_SUCCEEDED(ret)) {
                    parseFeed(response);
                } else {
                    updateStatus("Failed to load feed");
                }
            }
        } else {
            updateStatus("Invalid Code");
        }
    }
}

void render() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    
    C2D_TargetClear(top, CLR_TIKTOK_BG);
    C2D_SceneBegin(top);
    
    if (g_state == STATE_LOGIN) {
        // Logo
        C2D_DrawCircleSolid(200, 100, 0, 40, CLR_ACCENT_CYAN);
        C2D_DrawCircleSolid(205, 100, 0, 40, CLR_ACCENT_RED);
        C2D_DrawCircleSolid(202, 100, 0, 35, CLR_TIKTOK_BG); 
        
        C2D_DrawText(&g_textObj, C2D_WithColor, 80.0f, 180.0f, 0.5f, 0.6f, 0.6f, CLR_TIKTOK_TEXT);
    } 
    else if (g_state == STATE_FEED) {
        // Draw Feed List
        float y = 20.0f;
        for(int i=0; i<g_videoCount; i++) {
            u32 color = (i == g_selectedVideo) ? CLR_ACCENT_RED : CLR_TIKTOK_TEXT;
            
            C2D_TextBuf buf = C2D_TextBufNew(256);
            C2D_Text txt;
            C2D_TextParse(&txt, buf, g_videoTitles[i]);
            C2D_TextOptimize(&txt);
            
            C2D_DrawText(&txt, C2D_WithColor, 10.0f, y, 0.5f, 0.5f, 0.5f, color);
            
            // cleanup (inefficient per frame, but ok for demo)
            C2D_TextBufDelete(buf);
            y += 30.0f;
        }
        
        if(g_videoCount == 0) {
             C2D_DrawText(&g_textObj, C2D_WithColor, 10.0f, 100.0f, 0.5f, 0.5f, 0.5f, CLR_TIKTOK_TEXT);
        }
    }

    C2D_TargetClear(bottom, C2D_Color32(20,20,20,255));
    C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0, 320, 30, CLR_ACCENT_RED); // Header

    C3D_FrameEnd(0);
}

int main(int argc, char* argv[]) {
    initGraphics();
    network_init();
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;
        
        if (g_state == STATE_LOGIN) {
            if (kDown & KEY_A) {
                doLogin();
            }
        } else if (g_state == STATE_FEED) {
            if (kDown & KEY_DDOWN) {
                g_selectedVideo++;
                if(g_selectedVideo >= g_videoCount) g_selectedVideo = 0;
            }
            if (kDown & KEY_DUP) {
                g_selectedVideo--;
                if(g_selectedVideo < 0) g_selectedVideo = g_videoCount - 1;
            }
        }

        render();
    }

    network_exit();
    C2D_TextBufDelete(g_staticBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
