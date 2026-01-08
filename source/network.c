#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"

// Production Server URL
#define SERVER_URL "https://oscaritodev.pythonanywhere.com"

Result network_init() {
    return httpcInit(0);
}

void network_exit() {
    httpcExit();
}

// Simple Helper to perform a POST request with JSON body
// Returns the response body in out_buffer.
Result network_post(const char* endpoint, const char* json_body, char* out_buffer, size_t buffer_size) {
    Result ret = 0;
    httpcContext context;
    char url[256];
    
    snprintf(url, sizeof(url), "%s%s", SERVER_URL, endpoint);
    
    ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);
    if(R_FAILED(ret)) return ret;

    httpcAddRequestHeaderField(&context, "Content-Type", "application/json");

    // Add POST body before starting the request
    ret = httpcAddPostDataRaw(&context, (u32*)json_body, strlen(json_body));
    if(R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    ret = httpcBeginRequest(&context);
    if(R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    u32 statuscode = 0;
    ret = httpcGetResponseStatusCode(&context, &statuscode);
    if(R_FAILED(ret) || statuscode != 200) {
        httpcCloseContext(&context);
        return -1; // HTTP Error
    }

    u32 contentsize = 0;
    ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
    if(R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }
    
    // Safety check
    if (contentsize >= buffer_size) contentsize = buffer_size - 1;

    ret = httpcReceiveData(&context, (u8*)out_buffer, contentsize);
    if (R_SUCCEEDED(ret)) {
        out_buffer[contentsize] = '\0'; // Null terminate
    }

    httpcCloseContext(&context);
    return ret;
}

Result network_get(const char* endpoint, const char* token, char* out_buffer, size_t buffer_size) {
    Result ret = 0;
    httpcContext context;
    char url[256];
    
    snprintf(url, sizeof(url), "%s%s", SERVER_URL, endpoint);
    
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
    if(R_FAILED(ret)) return ret;

    if (token) {
        httpcAddRequestHeaderField(&context, "x-session-token", token);
    }

    ret = httpcBeginRequest(&context);
    if(R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }
    
    u32 statuscode = 0;
    ret = httpcGetResponseStatusCode(&context, &statuscode);
     if(R_FAILED(ret) || statuscode != 200) {
        httpcCloseContext(&context);
        return -2;
    }

    // We get the size first
    u32 contentsize = 0;
    httpcGetDownloadSizeState(&context, NULL, &contentsize);
    if (contentsize == 0 || contentsize >= buffer_size) contentsize = buffer_size - 1;

    ret = httpcReceiveData(&context, (u8*)out_buffer, contentsize);
    if (R_SUCCEEDED(ret)) {
        out_buffer[contentsize] = '\0';
    }

    httpcCloseContext(&context);
    return ret;
}
