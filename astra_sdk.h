/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */


#ifndef ASTRA_SDK_H
#define ASTRA_SDK_H

#ifdef _WIN32
    #define ASTRA_API    __declspec(dllexport)
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_API    __attribute__((visibility("default")))
    #define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#include <string>
#include <vector>

#include "common.h" 
#include "error.h" 
typedef void (*ErrorReportFn)(ErrCode code, int line, const std::string& name);

class AstraVM;

typedef void (*RegisterFunc)(const char* name, void (*func)(AstraVM*));

typedef void (*ChainStoreFn)(void* vm,
                             const std::string& chainName,
                             std::vector<Value> values,
                             std::vector<std::string> fields);

extern ChainStoreFn g_chainStore;  

// helpers
inline double toD(const Value& v) {
    return (v.type == VAL_FLOAT) ? v.decimal : (double)v.num;
}
inline Value makeInt(long long n) {
    Value v; v.type=VAL_INT; v.num=n; v.isInitialized=true; return v;
}
inline Value makeFloat(double d) {
    Value v; v.type=VAL_FLOAT; v.decimal=d; v.isInitialized=true; return v;
}
inline Value makeStr(const std::string& s) {
    Value v; v.type=VAL_STR; v.str=s; v.isInitialized=true; return v;
}
inline Value makeBool(bool b) {
    Value v; v.type=VAL_BOOL;
    v.tristate=b?AST_TRUE:AST_FALSE;
    v.isInitialized=true; return v;
}
inline Value createVal(ValueType t, long long n, double d) {
    Value v; v.type=t; v.isInitialized=true; v.num=n; v.decimal=d; return v;
}

#endif