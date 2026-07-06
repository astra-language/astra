/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "vm.h"
#include <iostream>

void AstraVM::chainStore(const std::string& chainName,
                          const std::vector<Value>& values,
                          const std::vector<std::string>& fields) {

    bool hasFields = !fields.empty();
    int fieldCount = hasFields ? (int)fields.size() : 1;
    int tupleCount = hasFields ? (int)values.size() / fieldCount
                               : (int)values.size();

    if (chainTable.find(chainName) == chainTable.end()) {
        ChainInfo info;
        info.startId      = vmNextVarId;
        info.count        = 0;
        info.totalSlots   = 0;
        info.namedOffset  = 0; 
        info.fields      = fields; 
        chainTable[chainName] = info;
    }
    
    if (values.empty()) return;
    ChainInfo& info  = chainTable[chainName];
    std::string base = chainName.substr(0, chainName.find(':'));
    int startSize = (int)info.simpleAddrs.size();
    if (!hasFields) {
    for (int t = 0; t < tupleCount; t++) {
        if (vmNextVarId >= MAX_MEMORY) {
            if (!hasFatalMemoryError) {
                AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                    "Memory limit exceeded (" + std::to_string(MAX_MEMORY) +
                    " variables/chain elements max)");
            }
            hasFatalMemoryError = true;
            break;   
        }

        int addr    = vmNextVarId;
        int flatIdx = startSize + t + 1;
        std::string flat = base + std::to_string(flatIdx);
        
        memory[addr]               = values[t];
        memory[addr].isInitialized = true;
        if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");
        varNames[addr]      = flat;
        vmSymbolTable[flat] = addr;

        TupleEntry entry;
        entry.startAddr  = addr;
        entry.fieldCount = 1;
        entry.hasFields  = false;
        info.tuples.push_back(entry);
        info.simpleAddrs.push_back(addr);

        vmNextVarId++;
    }
    info.totalSlots += (int)info.simpleAddrs.size() - startSize;
    info.count      += (int)info.simpleAddrs.size() - startSize;
} else {
    bool stoppedEarly = false;
    int actualTuples = 0;

    for (int t = 0; t < tupleCount; t++) {
        if (vmNextVarId + fieldCount > MAX_MEMORY) {
            if (!hasFatalMemoryError) {
                AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                    "Memory limit exceeded (" + std::to_string(MAX_MEMORY) +
                    " variables/chain elements max)");
            }
            hasFatalMemoryError = true;
            stoppedEarly = true;
            break;
        }

        int flatIdx = info.namedOffset + t + 1;
        std::string flat = base + std::to_string(flatIdx);

        for (int f = 0; f < fieldCount; f++) {
            int addr = vmNextVarId + f;

            memory[addr]               = values[t * fieldCount + f];
            memory[addr].isInitialized = true;
            if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");

            std::string fieldKey = flat + ":" + fields[f];
            vmSymbolTable[fieldKey] = addr;
            varNames[addr]          = fieldKey;
        }
        vmNextVarId += fieldCount;
        actualTuples++;
    }
    info.namedOffset += actualTuples;
    info.count       += actualTuples;
    info.totalSlots  += actualTuples;
}

    
if (!fields.empty()) {
    info.fields = fields;
}
}

void AstraVM::chainPrint(const std::string& chainName) {
    if (chainTable.find(chainName) == chainTable.end()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            " Chain '" + chainName + "' not found");
        return;
    }

    ChainInfo& info = chainTable[chainName];

    std::cout << "\033[32m";
    for (int i = 0; i < (int)info.simpleAddrs.size(); i++) {
        Value& v = memory[info.simpleAddrs[i]];
        if      (v.type == VAL_INT)   std::cout << v.num;
        else if (v.type == VAL_STR)   std::cout << v.str;
        else if (v.type == VAL_FLOAT) {
            std::string s = std::to_string(v.decimal);
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
            std::cout << s;
        }
        else if (v.type == VAL_BOOL)
            std::cout << (v.tristate == AST_TRUE ? "TRUE" : 
                  v.tristate == AST_MAYBE ? "MAYBE" : "FALSE");
        if (i < (int)info.simpleAddrs.size() - 1) std::cout << " ";
    }
    std::cout << "\033[0m\n";
}

Value AstraVM::chainGet(const std::string& chainName, int index) {
    if (chainTable.find(chainName) == chainTable.end()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            " Chain '" + chainName + "' not found");
        return {};
    }

    ChainInfo& info = chainTable[chainName];
    if (index < 1 || index > (int)info.tuples.size()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            " Chain index " + std::to_string(index) + " out of range");
        return {};
    }

    return memory[info.tuples[index - 1].startAddr];
}
