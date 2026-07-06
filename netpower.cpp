/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "astra_sdk.h"
#include "vm.h"
#include <curl/curl.h>
#include <string>
#include <fstream>
#include <iostream>

#ifdef _WIN32
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

// ── Helpers ──────────────────────────────────────────────────────────────────
Value mkStr(const std::string& s) {
    Value v; v.type = VAL_STR; v.str = s; v.isInitialized = true; return v;
}
Value mkInt(long long n) {
    Value v; v.type = VAL_INT; v.num = n; v.isInitialized = true; return v;
}


static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}


static size_t writeFileCallback(void* contents, size_t size, size_t nmemb, std::ofstream* out) {
    size_t totalSize = size * nmemb;
    out->write((char*)contents, totalSize);
    return totalSize;
}

// ── get(url) ─────────────────────────────────────────────────────────────────
void net_get(AstraVM* vm) {
    Value urlVal = vm->pop();

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (g_reportError) g_reportError(ErrCode::NET_INIT_FAILED, vm->currentLine, "get()");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, urlVal.str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Astra/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        if (g_reportError) g_reportError(ErrCode::NET_REQUEST_FAILED, vm->currentLine,
            std::string("GET failed - ") + curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }

    curl_easy_cleanup(curl);
    vm->push(mkStr(response));
}

// ── post(url, data) ──────────────────────────────────────────────────────────
void net_post(AstraVM* vm) {
    Value dataVal = vm->pop();
    Value urlVal  = vm->pop();

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (g_reportError) g_reportError(ErrCode::NET_INIT_FAILED, vm->currentLine, "post()");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, urlVal.str.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataVal.str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Astra/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        if (g_reportError) g_reportError(ErrCode::NET_REQUEST_FAILED, vm->currentLine,
            std::string("POST failed - ") + curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }

    curl_easy_cleanup(curl);
    vm->push(mkStr(response));
}

// ── status(url) ──────────────────────────────────────────────────────────────
void net_status(AstraVM* vm) {
    Value urlVal = vm->pop();

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (g_reportError) g_reportError(ErrCode::NET_INIT_FAILED, vm->currentLine, "status()");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, urlVal.str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Astra/1.0");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
        vm->push(mkInt((long long)httpCode));
    } else {
        if (g_reportError) g_reportError(ErrCode::NET_REQUEST_FAILED, vm->currentLine,
            std::string("status() failed - ") + curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── download(url, filepath) ─────────────────────────────────────────────────
void net_download(AstraVM* vm) {
    Value pathVal = vm->pop();
    Value urlVal  = vm->pop();

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (g_reportError) g_reportError(ErrCode::NET_INIT_FAILED, vm->currentLine, "download()");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    std::ofstream outFile(pathVal.str, std::ios::binary);
    if (!outFile.is_open()) {
        if (g_reportError) g_reportError(ErrCode::NET_FILE_ERROR, vm->currentLine, pathVal.str);
        curl_easy_cleanup(curl);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, urlVal.str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Astra/1.0");

    CURLcode res = curl_easy_perform(curl);
    outFile.close();

    if (res != CURLE_OK) {
        if (g_reportError) g_reportError(ErrCode::NET_REQUEST_FAILED, vm->currentLine,
            std::string("download() failed - ") + curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        //std::remove(pathVal.str.c_str()); 
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    curl_easy_cleanup(curl);
    vm->push(mkInt(1));
}

// ── Register ─────────────────────────────────────────────────────────────────
ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    reg("get",      net_get);
    reg("post",     net_post);
    reg("status",   net_status);
    reg("download", net_download);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "get(url)|HTTP GET request -> response body string\n"
            "post(url,data)|HTTP POST request -> response body string\n"
            "status(url)|HTTP status code -> int (200, 404, etc)\n"
            "download(url,file)|Download file from URL -> 1/0\n";
    }
    return "Net_Module_Active";
}