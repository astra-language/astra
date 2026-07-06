/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */


#include "astra_sdk.h"
#include "error.h"
#include "vm.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
#define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

// ------------------ FORMAT TIME ------------------
void print_formatted_time(std::time_t t) {
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    std::cout << oss.str() << std::endl;
}

// ------------------ SPLIT ------------------
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out;
    std::string temp;

    for (char c : s) {
        if (c == delim) {
            if (!temp.empty()) {
                out.push_back(temp);
                temp.clear();
            }
        } else {
            temp += c;
        }
    }

    if (!temp.empty())
        out.push_back(temp);

    return out;
}

// ------------------ TIME ------------------
void m_time(AstraVM* vm) {
    Value v1 = vm->pop();
    auto now = std::time(nullptr);

    // NO ARGUMENTS
    if (!v1.isInitialized) {
        print_formatted_time(now);
        return;
    }

    std::string arg = v1.str;

    // ---------------- NEW SYNTAX ----------------
    if (arg.find(':') != std::string::npos) {

        std::vector<std::string> parts = split(arg, ',');

        for (auto &p : parts) {

            size_t pos = p.find(':');
            if (pos == std::string::npos) {
                AstraError::syntax(ErrCode::INVALID_SYNTAX, 0, p);
                return;
            }

            std::string unit = p.substr(0, pos);
            long long value = 0;

            try {
                value = std::stoll(p.substr(pos + 1));
            } catch (...) {
                AstraError::type(ErrCode::TYPE_MISMATCH, 0, p);
                return;
            }

            if (unit == "h") now += value * 3600;
            else if (unit == "m") now += value * 60;
            else if (unit == "s") now += value;
            else {
                g_reportError(ErrCode::INVALID_TIME_UNIT, 0, unit);
                return;
            }
        }

        print_formatted_time(now);
        return;
    }

    // ---------------- OLD SYNTAX ----------------
    Value v2 = vm->pop();
    long long amount = (long long)v1.num;

    if (v2.str == "h") now += amount * 3600;
    else if (v2.str == "m") now += amount * 60;
    else if (v2.str == "s") now += amount;
    else {
        g_reportError(ErrCode::INVALID_TIME_UNIT, 0, v2.str);
        return;
    }

    print_formatted_time(now);
}

// ------------------ DATE ------------------
void m_date(AstraVM* vm) {
    Value v1 = vm->pop();
    auto now = std::time(nullptr);
    struct tm *tm = std::localtime(&now);

    // NO ARGUMENTS
    if (!v1.isInitialized) {
        std::ostringstream oss;
        oss << std::put_time(tm, "%d-%m-%Y");
        std::cout << oss.str() << std::endl;
        return;
    }

    std::string arg = v1.str;

    // ---------------- NEW SYNTAX ----------------
    if (arg.find(':') != std::string::npos) {

        std::vector<std::string> parts = split(arg, ',');

        for (auto &p : parts) {

            size_t pos = p.find(':');
            if (pos == std::string::npos) {
                AstraError::syntax(ErrCode::INVALID_SYNTAX, 0, p);
                return;
            }

            std::string unit = p.substr(0, pos);
            long long value = 0;

            try {
                value = std::stoll(p.substr(pos + 1));
            } catch (...) {
                AstraError::type(ErrCode::TYPE_MISMATCH, 0, p);
                return;
            }

            if (unit == "d") tm->tm_mday += value;
            else if (unit == "w") tm->tm_mday += value * 7;
            else if (unit == "m") tm->tm_mon += value;
            else if (unit == "y") tm->tm_year += value;
            else {
                g_reportError(ErrCode::INVALID_DATE_UNIT, 0, unit);
                return;
            }
        }

        now = std::mktime(tm);
        tm = std::localtime(&now);
    }

    // ---------------- OLD SYNTAX ----------------
    else {
        Value v2 = vm->pop();
        long long amount = (long long)v1.num;

        if (v2.str == "d") now += amount * 86400;
        else if (v2.str == "w") now += amount * 604800;
        else if (v2.str == "m") tm->tm_mon += amount;
        else if (v2.str == "y") tm->tm_year += amount;
        else {
            g_reportError(ErrCode::INVALID_DATE_UNIT, 0, v2.str);
            return;
        }

        now = std::mktime(tm);
        tm = std::localtime(&now);
    }

    std::ostringstream oss;
    oss << std::put_time(tm, "%d-%m-%Y");
    std::cout << oss.str() << std::endl;
}

// ------------------ SLEEP ------------------
void m_sleep(AstraVM* vm) {
    Value v1 = vm->pop();

    if (!v1.isInitialized) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, 0, "sleep missing argument");
        return;
    }

    long long seconds = 0;

    
    if (v1.type == VAL_INT) {
        seconds = v1.num;
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        return;
    }
    if (v1.type == VAL_FLOAT) {
        seconds = (long long)v1.decimal;
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        return;
    }

    // ── string format handling ──────────────
    std::string arg = v1.str;

    if (arg.find(':') != std::string::npos) {
        size_t pos = arg.find(':');
        std::string unit = arg.substr(0, pos);

        try {
            seconds = std::stoll(arg.substr(pos + 1));
        } catch (...) {
            AstraError::type(ErrCode::TYPE_MISMATCH, 0, arg);
            return;
        }

        if (unit != "s") {
            g_reportError(ErrCode::INVALID_TIME_UNIT, 0, unit);
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        return;
    }

    try {
        seconds = std::stoll(arg);
    } catch (...) {
        AstraError::type(ErrCode::TYPE_MISMATCH, 0, arg);
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}
void m_diff(AstraVM* vm) {
    Value v2 = vm->pop();
    Value v1 = vm->pop();

    if (v1.type != VAL_INT || v2.type != VAL_INT) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "diff() expects two integer timestamps");
        return;
    }

    long long diff = std::abs(v1.num - v2.num);

    long long days = diff / 86400;
    long long hours = (diff % 86400) / 3600;
    long long mins = (diff % 3600) / 60;
    long long secs = diff % 60;

    std::cout << days << "d " << hours << "h " << mins << "m " << secs << "s" << std::endl;
}

void m_timestamp(AstraVM* vm) {
    auto now = std::time(nullptr);
    Value v;
    v.type = VAL_INT;
    v.num = (long long)now;
    v.isInitialized = true;
    vm->push(v);
}

static std::chrono::steady_clock::time_point sw_start;
static bool sw_started = false;

void m_stopwatch(AstraVM* vm) {
    Value v = vm->pop();

    if (v.type != VAL_STR) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "stopwatch() expects \"start\" or \"stop\"");
        return;
    }

    if (v.str == "start") {
        sw_start = std::chrono::steady_clock::now();
        sw_started = true;
    } else if (v.str == "stop") {
        if (!sw_started) {
            g_reportError(ErrCode::INVALID_OPERATION, 0, "stopwatch stopped without start");
            return;
        }
        auto end = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end - sw_start).count();
        std::cout << elapsed << "s" << std::endl;
        sw_started = false;
    } else {
        g_reportError(ErrCode::INVALID_OPERATION, 0,
            "stopwatch() unknown argument '" + v.str + "', expected \"start\" or \"stop\"");
    }
}

void m_format(AstraVM* vm) {
    Value v = vm->pop();

    if (v.type != VAL_STR || v.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "format() expects a non-empty format string");
        return;
    }

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, v.str.c_str());
    std::cout << oss.str() << std::endl;
}

void m_addTime(AstraVM* vm) {
    Value offsetVal = vm->pop();
    Value baseVal = vm->pop();

    if (baseVal.type != VAL_INT) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "addtime() expects an integer base timestamp");
        return;
    }
    if (offsetVal.type != VAL_STR || offsetVal.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, 0,
            "addtime() expects an offset string like \"h:1,m:30\"");
        return;
    }

    long long base = baseVal.num;
    std::string arg = offsetVal.str;
    std::vector<std::string> parts = split(arg, ',');

    for (auto& p : parts) {
        size_t pos = p.find(':');
        if (pos == std::string::npos) {
            AstraError::syntax(ErrCode::INVALID_SYNTAX, 0,
                "addtime() malformed segment '" + p + "', expected unit:value");
            return;
        }

        std::string unit = p.substr(0, pos);
        long long value = 0;
        try {
            value = std::stoll(p.substr(pos + 1));
        } catch (...) {
            AstraError::type(ErrCode::TYPE_MISMATCH, 0, p);
            return;
        }

        if (unit == "h") base += value * 3600;
        else if (unit == "m") base += value * 60;
        else if (unit == "s") base += value;
        else if (unit == "d") base += value * 86400;
        else {
            g_reportError(ErrCode::INVALID_TIME_UNIT, 0, unit);
            return;
        }
    }

    Value v; v.type = VAL_INT; v.num = base; v.isInitialized = true;
    vm->push(v);
}

// ------------------ REGISTER ------------------
ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("time", m_time);
    reg("date", m_date);
    reg("sleep", m_sleep);
    reg("diff", m_diff);
    reg("timestamp", m_timestamp);
    reg("stopwatch", m_stopwatch);
    reg("format", m_format);
    reg("addtime", m_addTime);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "time()|Current time (HH:MM:SS)\n"
            "time(\"h:2,m:30\")|Time with offset\n"
            "date()|Current date (DD-MM-YYYY)\n"
            "date(\"d:5,m:1\")|Date with offset\n"
            "sleep(n)|Sleep n seconds\n"
            "sleep(\"s:4\")|Sleep with format\n"
            "diff(t1,t2)|Difference between timestamps\n"
            "timestamp()|Unix timestamp -> int\n"
            "stopwatch(\"start\")|Start stopwatch\n"
            "stopwatch(\"stop\")|Stop and print elapsed\n"
            "format(\"fmt\")|Custom date/time format\n"
            "addtime(base,\"h:1\")|Add offset to timestamp\n";
    }
    return "Time_Module_Active";
}