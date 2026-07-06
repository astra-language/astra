/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "builtfun.h"
#include <algorithm>
#include <iostream>
#include "vm.h"
#include "error.h"
#include <sstream>
#include <functional>

namespace BuiltinFunctions {

void chainLen(AstraVM* vm, const std::string& chainName) {
    Value v;
    v.type = VAL_INT;
    v.isInitialized = true;
    v.num = vm->chainTable.count(chainName) ? vm->chainTable[chainName].count : 0;
    vm->push(v);
}

void chainSort(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    
    std::sort(info.simpleAddrs.begin(), info.simpleAddrs.end(), 
        [&](int a, int b) {
            Value& va = vm->memory[a];
            Value& vb = vm->memory[b];
            if (va.type == VAL_INT)   return va.num < vb.num;
            if (va.type == VAL_FLOAT) return va.decimal < vb.decimal;
            if (va.type == VAL_STR)   return va.str < vb.str;
            return false;
        });

    std::vector<Value> vals;
    for (int addr : info.simpleAddrs) vals.push_back(vm->memory[addr]);
    for (int i = 0; i < (int)info.simpleAddrs.size(); i++)
        vm->memory[info.simpleAddrs[i]] = vals[i];
}

void chainMerge(AstraVM* vm, const std::string& chain1, const std::string& chain2) {
    if (!vm->chainTable.count(chain1)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chain1);
        return;
    }
    if (!vm->chainTable.count(chain2)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chain2);
        return;
    }
    ChainInfo& info1 = vm->chainTable[chain1];
    ChainInfo& info2 = vm->chainTable[chain2];
    
    for (int addr : info2.simpleAddrs) {
        info1.simpleAddrs.push_back(addr);
        info1.count++;
        info1.totalSlots++;
    }
}

void chainUnique(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    
    std::vector<int> newAddrs;
    std::unordered_set<std::string> seen;
    
    for (int addr : info.simpleAddrs) {
        Value& v = vm->memory[addr];
        
        std::string key;
        if (v.type == VAL_INT)   key = std::to_string(v.num);
        else if (v.type == VAL_FLOAT) key = std::to_string(v.decimal);
        else if (v.type == VAL_STR)   key = v.str;
        
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            newAddrs.push_back(addr);
        }
    }
    
    info.simpleAddrs = newAddrs;
    info.count = (int)newAddrs.size();
    info.totalSlots = info.count;
}

void chainSum(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    if (info.simpleAddrs.empty()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "sum() on empty chain '" + chainName + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value v; v.type = VAL_FLOAT; v.isInitialized = true; v.decimal = 0.0;
    double sum = 0; bool allInt = true;
    for (int addr : info.simpleAddrs) {
        Value& val = vm->memory[addr];
        if (val.type == VAL_FLOAT) { sum += val.decimal; allInt = false; }
        else sum += (double)val.num;
    }
    if (allInt) { v.type = VAL_INT; v.num = (long long)sum; }
    else v.decimal = sum;
    vm->push(v);
}

void chainAvg(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    if (info.simpleAddrs.empty()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "avg() on empty chain '" + chainName + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value v; v.type = VAL_FLOAT; v.isInitialized = true;
    double sum = 0;
    for (int addr : info.simpleAddrs) {
        Value& val = vm->memory[addr];
        sum += (val.type == VAL_FLOAT) ? val.decimal : (double)val.num;
    }
    v.decimal = sum / info.simpleAddrs.size();
    vm->push(v);
}

void chainMax(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    if (info.simpleAddrs.empty()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "cmax() on empty chain '" + chainName + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value best = vm->memory[info.simpleAddrs[0]];
    for (int addr : info.simpleAddrs) {
        Value& val = vm->memory[addr];
        double a = (val.type == VAL_FLOAT) ? val.decimal : (double)val.num;
        double b = (best.type == VAL_FLOAT) ? best.decimal : (double)best.num;
        if (a > b) best = val;
    }
    vm->push(best);
}

void chainMin(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    if (info.simpleAddrs.empty()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "cmin() on empty chain '" + chainName + "'");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value best = vm->memory[info.simpleAddrs[0]];
    for (int addr : info.simpleAddrs) {
        Value& val = vm->memory[addr];
        double a = (val.type == VAL_FLOAT) ? val.decimal : (double)val.num;
        double b = (best.type == VAL_FLOAT) ? best.decimal : (double)best.num;
        if (a < b) best = val;
    }
    vm->push(best);
}

void chainContains(AstraVM* vm, const std::string& chainName, const Value& target) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value v; v.type = VAL_BOOL; v.isInitialized = true; v.tristate = AST_FALSE;
    ChainInfo& info = vm->chainTable[chainName];
    for (int addr : info.simpleAddrs) {
        Value& val = vm->memory[addr];
        bool match = false;
        if (val.type == VAL_INT && target.type == VAL_INT) match = (val.num == target.num);
        else if (val.type == VAL_STR && target.type == VAL_STR) match = (val.str == target.str);
        else if (val.type == VAL_FLOAT && target.type == VAL_FLOAT) match = (val.decimal == target.decimal);
        if (match) { v.tristate = AST_TRUE; break; }
    }
    vm->push(v);
}

void chainIndexOf(AstraVM* vm, const std::string& chainName, const Value& target) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value v; v.type = VAL_INT; v.isInitialized = true; v.num = -1;
    ChainInfo& info = vm->chainTable[chainName];
    for (size_t i = 0; i < info.simpleAddrs.size(); i++) {
        Value& val = vm->memory[info.simpleAddrs[i]];
        bool match = false;
        if (val.type == VAL_INT && target.type == VAL_INT) match = (val.num == target.num);
        else if (val.type == VAL_STR && target.type == VAL_STR) match = (val.str == target.str);
        else if (val.type == VAL_FLOAT && target.type == VAL_FLOAT) match = (val.decimal == target.decimal);
        if (match) { v.num = (long long)(i + 1); break; }
    }
    vm->push(v);
}

void chainReverse(AstraVM* vm, const std::string& chainName) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        return;
    }
    ChainInfo& info = vm->chainTable[chainName];
    std::reverse(info.simpleAddrs.begin(), info.simpleAddrs.end());
}

void chainJoin(AstraVM* vm, const std::string& chainName, const std::string& delim) {
    if (!vm->chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        vm->push(poison);
        return;
    }
    Value v; v.type = VAL_STR; v.isInitialized = true; v.str = "";
    ChainInfo& info = vm->chainTable[chainName];
    std::string result;
    for (size_t i = 0; i < info.simpleAddrs.size(); i++) {
        Value& val = vm->memory[info.simpleAddrs[i]];
        if (val.type == VAL_INT) result += std::to_string(val.num);
        else if (val.type == VAL_STR) result += val.str;
        else if (val.type == VAL_FLOAT) result += std::to_string(val.decimal);
        if (i < info.simpleAddrs.size() - 1) result += delim;
    }
    v.str = result;
    vm->push(v);
}

void adr(AstraVM* vm, const std::string& varName) {
    int index = vm->getVariableAddress(varName);
    
    uintptr_t rawAddress = reinterpret_cast<uintptr_t>(&vm->memory[index]);
    
    std::stringstream ss;
    ss << "0x" << std::hex << rawAddress;
    
    Value v;
    v.type = VAL_PTR; 
    v.str = ss.str();
    v.isInitialized = true;
    vm->push(v);
}

void val(AstraVM* vm, Value addrVal) {
    if (addrVal.type == VAL_PTR) {
        uintptr_t rawAddress = std::stoull(addrVal.str, nullptr, 16);
        Value* v = reinterpret_cast<Value*>(rawAddress);
        vm->push(*v);
    } 
    else {
        vm->push(addrVal); 
    }
}
void jsonPretty(AstraVM* vm, const std::string& jsonStr) {
    const std::string& s = jsonStr;
    size_t i = 0;
    const int INDENT_SIZE = 4;
    bool malformed = false;

    auto skipSpaces = [&]() {
        while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
    };

    auto indent = [&](int depth) {
        return std::string((size_t)depth * INDENT_SIZE, ' ');
    };

    std::function<std::string()> readRawString = [&]() -> std::string {
        std::string out;
        if (i >= s.size() || s[i] != '"') { malformed = true; return out; }
        out += s[i++];
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) { out += s[i]; out += s[i+1]; i += 2; }
            else { out += s[i]; i++; }
        }
        if (i < s.size()) { out += s[i]; i++; } 
        else malformed = true;
        return out;
    };

    std::function<std::string(int)> formatValue = [&](int depth) -> std::string {
        skipSpaces();
        if (i >= s.size()) { malformed = true; return ""; }

        char c = s[i];

        if (c == '"') {
            return readRawString();
        }
        else if (c == '{') {
            i++; 
            std::string out = "{\n";
            skipSpaces();
            bool first = true;
            while (i < s.size() && s[i] != '}') {
                if (!first) out += ",\n";
                first = false;
                skipSpaces();
                out += indent(depth + 1);
                out += readRawString();     
                if (malformed) return out;
                skipSpaces();
                if (i < s.size() && s[i] == ':') i++;
                else { malformed = true; return out; }
                skipSpaces();
                out += ": ";
                out += formatValue(depth + 1);
                skipSpaces();
                if (i < s.size() && s[i] == ',') { i++; skipSpaces(); }
            }
            if (i < s.size() && s[i] == '}') i++;
            else { malformed = true; }
            out += "\n" + indent(depth) + "}";
            return out;
        }
        else if (c == '[') {
            i++; 
            std::string out = "[\n";
            skipSpaces();
            bool first = true;
            while (i < s.size() && s[i] != ']') {
                if (!first) out += ",\n";
                first = false;
                out += indent(depth + 1);
                out += formatValue(depth + 1);
                skipSpaces();
                if (i < s.size() && s[i] == ',') { i++; skipSpaces(); }
            }
            if (i < s.size() && s[i] == ']') i++;
            else { malformed = true; }
            out += "\n" + indent(depth) + "]";
            return out;
        }
        else {
            std::string out;
            while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' &&
                   !std::isspace((unsigned char)s[i])) {
                out += s[i++];
            }
            if (out.empty()) malformed = true;
            return out;
        }
    };

    std::string result = formatValue(0);

    if (malformed) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, vm->currentLine,
            "jsonpretty() received malformed JSON near position " + std::to_string(i));
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR;
        vm->push(poison);
        return;
    }

    Value v; v.type = VAL_STR; v.isInitialized = true; v.str = result;
    vm->push(v);
}

}