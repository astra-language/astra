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
#include <algorithm>
#include <sstream>
#include <vector>

#ifdef _WIN32
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_EXPORT extern "C"
#endif

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

// ── Helpers ──────────────────────────────────────────
std::string getStr(AstraVM* vm) {
    Value v = vm->pop();
    if (v.isPoisoned) {          
        return "";
    }
    if (v.type == VAL_STR) return v.str;
    if (v.type == VAL_INT) return std::to_string(v.num);
    if (v.type == VAL_FLOAT) {
        std::string s = std::to_string(v.decimal);
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }
    g_reportError(ErrCode::TYPE_MISMATCH, 0,
        "expected a string/number argument, got an unsupported type");
    return "";
}
static long long getInt(AstraVM* vm, const char* funcName, const char* argName) {
    Value v = vm->pop();
    if (v.type != VAL_INT) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            std::string(funcName) + "() expects an integer for '" + argName + "'");
        return 0;
    }
    return v.num;
}


// ── Functions ─────────────────────────────────────────
void t_upper(AstraVM* vm) {
    std::string s = getStr(vm);
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    vm->push(makeStr(s));
}

void t_lower(AstraVM* vm) {
    std::string s = getStr(vm);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    vm->push(makeStr(s));
}

void t_trim(AstraVM* vm) {
    std::string s = getStr(vm);
    // left trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
    }));
    // right trim
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
    vm->push(makeStr(s));
}

void t_len(AstraVM* vm) {
    std::string s = getStr(vm);
    vm->push(makeInt((long long)s.size()));
}

void t_replace(AstraVM* vm) {
    std::string rep = getStr(vm);
    std::string pat = getStr(vm);
    std::string s   = getStr(vm);

    if (pat.empty()) {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "replace() pattern cannot be empty (would loop infinitely)");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }

    size_t pos = 0;
    while ((pos = s.find(pat, pos)) != std::string::npos) {
        s.replace(pos, pat.size(), rep);
        pos += rep.size();
    }
    vm->push(makeStr(s));
}

void t_reverse(AstraVM* vm) {
    std::string s = getStr(vm);
    std::reverse(s.begin(), s.end());
    vm->push(makeStr(s));
}

void t_contains(AstraVM* vm) {
    std::string pat = getStr(vm);
    std::string s   = getStr(vm);
    vm->push(makeBool(s.find(pat) != std::string::npos));
}

void t_startswith(AstraVM* vm) {
    std::string pat = getStr(vm);
    std::string s   = getStr(vm);
    vm->push(makeBool(s.size() >= pat.size() && s.substr(0, pat.size()) == pat));
}

void t_endswith(AstraVM* vm) {
    std::string pat = getStr(vm);
    std::string s   = getStr(vm);
    vm->push(makeBool(s.size() >= pat.size() && 
              s.substr(s.size() - pat.size()) == pat));
}

void t_tonum(AstraVM* vm) {
    std::string s = getStr(vm);
    if (s.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0, "tonum() received an empty string");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }
    try {
        if (s.find('.') != std::string::npos) {
            Value v; v.type = VAL_FLOAT;
            v.decimal = std::stod(s);
            v.isInitialized = true;
            vm->push(v);
        } else {
            vm->push(makeInt(std::stoll(s)));
        }
    } catch (...) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "tonum() could not convert '" + s + "' to a number");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
    }
}

void t_tostr(AstraVM* vm) {
    Value v = vm->pop();
    if (v.type == VAL_INT) {
        vm->push(makeStr(std::to_string(v.num)));
    }
    else if (v.type == VAL_FLOAT) {
        std::string s = std::to_string(v.decimal);
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
        vm->push(makeStr(s));
    }
    else if (v.type == VAL_BOOL) {
        vm->push(makeStr(v.tristate == AST_TRUE ? "TRUE" :
                          v.tristate == AST_FALSE ? "FALSE" : "MAYBE"));
    }
    else if (v.type == VAL_STR) {
        vm->push(v);
    }
    else {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "tostr() received an unsupported type");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
    }
}

void t_substr(AstraVM* vm) {
    long long len   = getInt(vm, "substr", "len");
    long long start = getInt(vm, "substr", "start");
    std::string s   = getStr(vm);

    if (start < 0) start = 0;
    if (len < 0) {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "substr() length cannot be negative");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    if (start >= (long long)s.size()) { vm->push(makeStr("")); return; }

    vm->push(makeStr(s.substr((size_t)start, (size_t)len)));
}

void t_indexof(AstraVM* vm) {
    std::string pat = getStr(vm);
    std::string s   = getStr(vm);
    size_t pos = s.find(pat);
    vm->push(makeInt(pos == std::string::npos ? -1 : (long long)pos));
}

void t_repeat(AstraVM* vm) {
    long long n   = getInt(vm, "strrepeat", "n");
    std::string s = getStr(vm);

    if (n < 0) {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "strrepeat() count cannot be negative");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }
    
    const long long MAX_RESULT_BYTES = 50LL * 1024 * 1024;
    if (!s.empty() && n * (long long)s.size() > MAX_RESULT_BYTES) {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "strrepeat() result too large (limit ~50MB)");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        return;
    }

    std::string result;
    result.reserve((size_t)(n * s.size()));
    for (long long i = 0; i < n; i++) result += s;
    vm->push(makeStr(result));
}
void t_split(AstraVM* vm) {
    std::string delim = getStr(vm);
    std::string s     = getStr(vm);

    if (delim.empty()) {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "split() delimiter cannot be empty");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison); }
        { Value poison2; poison2.isPoisoned = true; poison2.type = VAL_INT; vm->push(poison2); }
        return;
    }

    std::vector<std::string> parts;
    size_t pos = 0, found;
    while ((found = s.find(delim, pos)) != std::string::npos) {
        parts.push_back(s.substr(pos, found - pos));
        pos = found + delim.size();
    }
    if (pos <= s.size()) parts.push_back(s.substr(pos));

    for (auto& p : parts) vm->push(makeStr(p));
    vm->push(makeInt((long long)parts.size()));
}

void t_chars(AstraVM* vm) {
    std::string s = getStr(vm);

    if (s.empty()) {
        vm->push(makeInt(0));
        return;
    }

    for (char c : s) vm->push(makeStr(std::string(1, c)));
    vm->push(makeInt((long long)s.size()));
}

// ── Registration ──────────────────────────────────────
ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("upper",      t_upper);
    reg("lower",      t_lower);
    reg("trim",       t_trim);
    reg("slen",       t_len);
    reg("replace",    t_replace);
    reg("strrev",    t_reverse);
    reg("strcon",   t_contains);
    reg("startswith", t_startswith);
    reg("endswith",   t_endswith);
    reg("tonum",      t_tonum);
    reg("tostr",      t_tostr);
    reg("substr",     t_substr);
    reg("strind",    t_indexof);
    reg("strrepeat",  t_repeat);
    reg("split", t_split);
    reg("chars", t_chars);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "upper(s)|Convert to uppercase\n"
            "lower(s)|Convert to lowercase\n"
            "trim(s)|Remove leading/trailing spaces\n"
            "slen(s)|String length\n"
            "replace(s,pat,rep)|Replace pattern in string\n"
            "strrev(s)|Reverse string\n"
            "strcon(s,pat)|Check if contains -> 1/0\n"
            "startswith(s,pat)|Check if starts with -> 1/0\n"
            "endswith(s,pat)|Check if ends with -> 1/0\n"
            "tonum(s)|String to number\n"
            "tostr(x)|Number to string\n"
            "substr(s,start,len)|Substring\n"
            "indexof(s,pat)|Find index of pattern\n"
            "strrepeat(s,n)|Repeat string n times\n"
            "split(s,delim)|Split string by delimiter\n"
            "chars(s)|Split into individual characters\n";
    }
    return "Text_Module_Active";
}