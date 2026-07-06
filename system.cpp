/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "astra_sdk.h"
#include "vm.h"
#include "error.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

#ifdef _WIN32
    #include <windows.h>
    #include <lmcons.h>
    #include <sysinfoapi.h>
    #include <direct.h>
#else
    #include <unistd.h>
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <pwd.h>
    #include <limits.h>
#endif

#ifdef _WIN32
#define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
#define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace fs = std::filesystem;

// ── Helpers ──────────────────────────────────────────────────────────────────

Value mkStr(const std::string& s) {
    Value v; v.type = VAL_STR; v.str = s; v.isInitialized = true; return v;
}
Value mkInt(long long n) {
    Value v; v.type = VAL_INT; v.num = n; v.isInitialized = true; return v;
}

// ── os() ─────────────────────────────────────────────────────────────────────
void s_os(AstraVM* vm) {
#ifdef _WIN32
    vm->push(mkStr("Windows"));
#elif __APPLE__
    vm->push(mkStr("macOS"));
#elif __linux__
    struct utsname u;
    uname(&u);
    vm->push(mkStr(std::string(u.sysname) + " " + u.release));
#else
    vm->push(mkStr("Unknown"));
#endif
}

// ── cpu() ────────────────────────────────────────────────────────────────────
void s_cpu(AstraVM* vm) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    vm->push(mkStr("Cores: " + std::to_string(si.dwNumberOfProcessors)));
#elif __linux__
    std::ifstream f("/proc/cpuinfo");
    std::string line, name;
    while (std::getline(f, line)) {
        if (line.find("model name") != std::string::npos) {
            name = line.substr(line.find(":") + 2);
            break;
        }
    }
    vm->push(mkStr(name.empty() ? "Unknown CPU" : name));
#else
    vm->push(mkStr("Unknown CPU"));
#endif
}

// ── ram() ────────────────────────────────────────────────────────────────────
void s_ram(AstraVM* vm) {
#ifdef _WIN32
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    long long total = ms.ullTotalPhys / (1024 * 1024);
    long long avail = ms.ullAvailPhys / (1024 * 1024);
    vm->push(mkStr("Total: " + std::to_string(total) + "MB  Free: " + std::to_string(avail) + "MB"));
#elif __linux__
    struct sysinfo si;
    sysinfo(&si);
    long long total = (si.totalram * si.mem_unit) / (1024 * 1024);
    long long free  = (si.freeram  * si.mem_unit) / (1024 * 1024);
    vm->push(mkStr("Total: " + std::to_string(total) + "MB  Free: " + std::to_string(free) + "MB"));
#else
    vm->push(mkStr("Unknown RAM"));
#endif
}

// ── pid() ────────────────────────────────────────────────────────────────────
void s_pid(AstraVM* vm) {
#ifdef _WIN32
    vm->push(mkInt(GetCurrentProcessId()));
#else
    vm->push(mkInt(getpid()));
#endif
}

// ── cwd() ────────────────────────────────────────────────────────────────────
void s_cwd(AstraVM* vm) {
    vm->push(mkStr(fs::current_path().string()));
}

// ── mkdir("name") ────────────────────────────────────────────────────────────
void s_mkdir(AstraVM* vm) {
    Value v = vm->pop();
    try {
        fs::create_directories(v.str);
        vm->push(mkInt(1));
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "mkdir() failed to create directory '" + v.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── rmdir("name") ────────────────────────────────────────────────────────────
void s_rmdir(AstraVM* vm) {
    Value v = vm->pop();
    try {
        size_t n = fs::remove_all(v.str);
        vm->push(mkInt(n > 0 ? 1 : 0));
       } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "rm() failed to delete '" + v.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── exists("path") ───────────────────────────────────────────────────────────
void s_exists(AstraVM* vm) {
    Value v = vm->pop();
    vm->push(mkInt(fs::exists(v.str) ? 1 : 0));
}

// ── run("cmd") ───────────────────────────────────────────────────────────────
void s_run(AstraVM* vm) {
    Value v = vm->pop();
    int ret = std::system(v.str.c_str());
    vm->push(mkInt(ret));
}

// ── env("VAR") ───────────────────────────────────────────────────────────────
void s_env(AstraVM* vm) {
    Value v = vm->pop();
    const char* val = std::getenv(v.str.c_str());
    vm->push(mkStr(val ? val : ""));
}

// ── exit(0) ──────────────────────────────────────────────────────────────────
void s_exit(AstraVM* vm) {
    Value v = vm->pop();
    std::exit((int)v.num);
}

// ── listdir(".") ─────────────────────────────────────────────────────────────
void s_listdir(AstraVM* vm) {
    Value v = vm->pop();
    std::string result;
    try {
        for (auto& entry : fs::directory_iterator(v.str)) {
            result += entry.path().filename().string() + "\n";
        }
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "listdir() failed to list '" + v.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }
    vm->push(mkStr(result));
}

// ── rename("a","b") ──────────────────────────────────────────────────────────
void s_rename(AstraVM* vm) {
    Value dst = vm->pop();
    Value src = vm->pop();
       try {
        if (fs::exists(dst.str)) {
            if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "rename() destination already exists: '" + dst.str + "'");
            Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
            return;
        }
        fs::rename(src.str, dst.str);
        vm->push(mkInt(1));
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "rename() failed from '" + src.str + "' to '" + dst.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── copy("a","b") ────────────────────────────────────────────────────────────
void s_copy(AstraVM* vm) {
    Value dst = vm->pop();
    Value src = vm->pop();
       try {
        fs::copy(src.str, dst.str, fs::copy_options::overwrite_existing);
        vm->push(mkInt(1));
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "copy() failed from '" + src.str + "' to '" + dst.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── move("a","b") ────────────────────────────────────────────────────────────
void s_move(AstraVM* vm) {
    Value dst = vm->pop();
    Value src = vm->pop();
      try {
        fs::rename(src.str, dst.str);
        vm->push(mkInt(1));
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "move() failed from '" + src.str + "' to '" + dst.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
    }
}

// ── abspath(".") ─────────────────────────────────────────────────────────────
void s_abspath(AstraVM* vm) {
    Value v = vm->pop();
    try {
        vm->push(mkStr(fs::absolute(v.str).string()));
    } catch (...) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "abspath() failed for '" + v.str + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
    }
}

// ── hostname() ───────────────────────────────────────────────────────────────
void s_hostname(AstraVM* vm) {
#ifdef _WIN32
    char buf[256]; DWORD sz = 256;
    GetComputerNameA(buf, &sz);
    vm->push(mkStr(buf));
#else
    char buf[256];
    gethostname(buf, sizeof(buf));
    vm->push(mkStr(buf));
#endif
}

// ── username() ───────────────────────────────────────────────────────────────
void s_username(AstraVM* vm) {
#ifdef _WIN32
    char buf[UNLEN + 1]; DWORD sz = UNLEN + 1;
    GetUserNameA(buf, &sz);
    vm->push(mkStr(buf));
#else
    struct passwd* pw = getpwuid(getuid());
    vm->push(mkStr(pw ? pw->pw_name : "unknown"));
#endif
}

// ── tempdir() ────────────────────────────────────────────────────────────────
void s_tempdir(AstraVM* vm) {
    vm->push(mkStr(fs::temp_directory_path().string()));
}

// ── uptime() ─────────────────────────────────────────────────────────────────
void s_uptime(AstraVM* vm) {
#ifdef _WIN32
    ULONGLONG ms = GetTickCount64();
    long long secs = ms / 1000;
    long long h = secs / 3600, m = (secs % 3600) / 60, s = secs % 60;
    vm->push(mkStr(std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s"));
#elif __linux__
    struct sysinfo si;
    sysinfo(&si);
    long long h = si.uptime / 3600, m = (si.uptime % 3600) / 60, s = si.uptime % 60;
    vm->push(mkStr(std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s"));
#else
    vm->push(mkStr("Unknown"));
#endif
}

// ── Register ─────────────────────────────────────────────────────────────────
ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("os",       s_os);
    reg("cpu",      s_cpu);
    reg("ram",      s_ram);
    reg("pid",      s_pid);
    reg("cwd",      s_cwd);
    reg("mkdir",    s_mkdir);
    reg("rm",    s_rmdir);
    reg("exists",   s_exists);
    reg("run",      s_run);
    reg("env",      s_env);
    reg("exit",     s_exit);
    reg("listdir",  s_listdir);
    reg("rename",   s_rename);
    reg("copy",     s_copy);
    reg("move",     s_move);
    reg("abspath",  s_abspath);
    reg("hostname", s_hostname);
    reg("username", s_username);
    reg("tempdir",  s_tempdir);
    reg("uptime",   s_uptime);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "os()|Operating system name\n"
            "cpu()|CPU info\n"
            "ram()|RAM total and free\n"
            "pid()|Current process ID\n"
            "cwd()|Current working directory\n"
            "mkdir(path)|Create directory\n"
            "rm(path)|Delete directory/file\n"
            "exists(path)|Check if path exists -> 1/0\n"
            "run(cmd)|Execute shell command\n"
            "env(var)|Get environment variable\n"
            "exit(code)|Exit program\n"
            "listdir(path)|List directory contents\n"
            "rename(src,dst)|Rename file or folder\n"
            "copy(src,dst)|Copy file\n"
            "move(src,dst)|Move file\n"
            "abspath(path)|Get absolute path\n"
            "hostname()|Machine hostname\n"
            "username()|Current username\n"
            "tempdir()|Temp directory path\n"
            "uptime()|System uptime\n";
    }
    return "System_Module_Active";
}