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
#include <cmath>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

#ifdef _WIN32
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

void m_sqrt(AstraVM* vm) {
    Value v = vm->pop();
    double x = toD(v);
    if (x < 0) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "sqrt() of negative number (" + std::to_string(x) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_FLOAT; vm->push(poison); }
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, sqrt(x)));
}

void m_abs(AstraVM* vm)   { Value v = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, std::abs(toD(v)))); }
void m_sin(AstraVM* vm)   { Value v = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, sin(toD(v) * (3.14159265/180.0)))); }
void m_cos(AstraVM* vm)   { Value v = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, cos(toD(v) * (3.14159265/180.0)))); }

static constexpr double ASTRA_PI = 3.14159265358979323846;

void m_tan(AstraVM* vm) {
    Value v = vm->pop();
    double deg = toD(v);
    double rad = deg * (ASTRA_PI / 180.0);
    double c = cos(rad);
    if (std::abs(c) < 1e-8) {  
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "tan() undefined at " + std::to_string(deg) + " degrees (90°, 270°, ...)");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_FLOAT; vm->push(poison); }
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, tan(rad)));
}

void m_floor(AstraVM* vm) { Value v = vm->pop(); vm->push(createVal(VAL_INT, (long long)floor(toD(v)), 0.0)); }
void m_ceil(AstraVM* vm)  { Value v = vm->pop(); vm->push(createVal(VAL_INT, (long long)ceil(toD(v)), 0.0)); }
void m_round(AstraVM* vm) { Value v = vm->pop(); vm->push(createVal(VAL_INT, (long long)round(toD(v)), 0.0)); }

void m_log(AstraVM* vm) {
    Value v = vm->pop();
    double x = toD(v);
    if (x <= 0) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "log() of non-positive number (" + std::to_string(x) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_FLOAT; vm->push(poison); }
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, log10(x)));
}

void m_pow(AstraVM* vm) {
    Value v2 = vm->pop(); Value v1 = vm->pop();
    double base = toD(v1), exp_ = toD(v2);
    if (base < 0 && std::floor(exp_) != exp_) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "pow() of negative base with fractional exponent");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_FLOAT; vm->push(poison); }
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, pow(base, exp_)));
}

void m_mod(AstraVM* vm) {
    Value v2 = vm->pop(); Value v1 = vm->pop();
    double a = toD(v1), b = toD(v2);

    if (b == 0) {
        Value res;
        res.type = VAL_STR;
        res.str = "INFINITE";
        res.isInitialized = true;
        vm->push(res);
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, fmod(a, b)));
}

void m_max(AstraVM* vm) { Value v2 = vm->pop(); Value v1 = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, std::max(toD(v1), toD(v2)))); }
void m_min(AstraVM* vm) { Value v2 = vm->pop(); Value v1 = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, std::min(toD(v1), toD(v2)))); }

void m_rand(AstraVM* vm) {
    Value v2 = vm->pop(); Value v1 = vm->pop();
    int lo = (int)toD(v1), hi = (int)toD(v2);
    if (lo > hi) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            "rand() min (" + std::to_string(lo) + ") greater than max (" + std::to_string(hi) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }
    static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<> dis(lo, hi);
    vm->push(createVal(VAL_INT, dis(gen), 0.0));
}

void m_pi(AstraVM* vm)  { vm->push(createVal(VAL_FLOAT, 0, 3.141592653589793)); }
void m_e(AstraVM* vm)   { vm->push(createVal(VAL_FLOAT, 0, 2.718281828459045)); }
void m_exp(AstraVM* vm) { Value v = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, exp(toD(v)))); }

void m_ln(AstraVM* vm) {
    Value v = vm->pop();
    double x = toD(v);
    if (x <= 0) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "ln() of non-positive number (" + std::to_string(x) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_FLOAT; vm->push(poison); }
        return;
    }
    vm->push(createVal(VAL_FLOAT, 0, log(x)));
}

void m_cbrt(AstraVM* vm)  { Value v = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, cbrt(toD(v)))); }
void m_hypot(AstraVM* vm) { Value v2 = vm->pop(); Value v1 = vm->pop(); vm->push(createVal(VAL_FLOAT, 0, hypot(toD(v1), toD(v2)))); }

void m_fact(AstraVM* vm) {
    Value val = vm->pop();
    long long n = (long long)toD(val);

    if (n < 0) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "fact() of negative number (" + std::to_string(n) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }
    if (n > 20) {   
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            "fact() input too large (" + std::to_string(n) + "), max supported is 20");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }

    long long f = 1;
    for (long long i = 1; i <= n; i++) f *= i;
    vm->push(createVal(VAL_INT, f, 0.0));
}

void m_prime(AstraVM* vm) {
    long long n = (long long)toD(vm->pop());
    bool p = (n > 1);
    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0) { p = false; break; }
    }
    vm->push(createVal(VAL_INT, p ? 1 : 0, 0.0));
}

void m_gcd(AstraVM* vm) {
    Value v2 = vm->pop(); Value v1 = vm->pop();
    long long a = (long long)toD(v1), b = (long long)toD(v2);

    if (a == 0 && b == 0) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "gcd(0, 0) is undefined");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }

    while (b != 0) { long long t = b; b = a % b; a = t; }
    vm->push(createVal(VAL_INT, std::abs(a), 0.0));
}

void m_fib(AstraVM* vm) {
    long long n = (long long)toD(vm->pop());

    if (n < 0) {
        if (g_reportError) g_reportError(ErrCode::MATH_DOMAIN_ERROR, vm->currentLine,
            "fib() of negative number (" + std::to_string(n) + ")");
        { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); }
        return;
    }
    if (n <= 0) { vm->push(createVal(VAL_INT, 0, 0.0)); return; }

    long long a = 0, b = 1;
    for (long long i = 2; i <= n; i++) {
        long long c = a + b;
        a = b; b = c;
    }
    vm->push(createVal(VAL_INT, b, 0.0));
}

ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("sqrt", m_sqrt); reg("abs", m_abs); reg("sin", m_sin); reg("cos", m_cos);
    reg("tan", m_tan); reg("floor", m_floor); reg("ceil", m_ceil); reg("round", m_round);
    reg("log", m_log); reg("pow", m_pow); reg("mod", m_mod); reg("max", m_max);
    reg("min", m_min); reg("rand", m_rand); reg("pi", m_pi); reg("e", m_e);
    reg("exp", m_exp); reg("ln", m_ln); reg("cbrt", m_cbrt); reg("hypot", m_hypot);
    reg("fact", m_fact); reg("prime", m_prime); reg("gcd", m_gcd); reg("fib", m_fib);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "sqrt(x)|Square root\n"
            "abs(x)|Absolute value\n"
            "sin(x)|Sine (degrees)\n"
            "cos(x)|Cosine (degrees)\n"
            "tan(x)|Tangent (degrees)\n"
            "floor(x)|Round down\n"
            "ceil(x)|Round up\n"
            "round(x)|Round nearest\n"
            "log(x)|Log base 10\n"
            "ln(x)|Natural log\n"
            "exp(x)|e^x\n"
            "pow(x,y)|x to power y\n"
            "mod(x,y)|Remainder\n"
            "max(x,y)|Maximum\n"
            "min(x,y)|Minimum\n"
            "rand(x,y)|Random integer\n"
            "pi()|Pi value\n"
            "e()|Euler number\n"
            "cbrt(x)|Cube root\n"
            "hypot(x,y)|Hypotenuse\n"
            "fact(x)|Factorial\n"
            "prime(x)|Is prime -> 1/0\n"
            "gcd(x,y)|Greatest common divisor\n"
            "fib(x)|Fibonacci number\n";
    }
    return "Math_Module_Active";
}