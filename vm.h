/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef VM_H
#define VM_H

#include "common.h"
#include "compiler.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include "PowerManager.h"
#include <fstream>
#include "error.h"

struct CallFrame {
    int returnAddress;
    int basePointer;
    int savedVmNextVarId;
    std::string savedSelf;
};

struct TupleEntry {
    int startAddr;      
    int fieldCount;    
    bool hasFields;     
};

struct ChainInfo {
    int startId     = 0;
    int count       = 0;
    int totalSlots  = 0;
    int namedOffset = 0;  
    std::vector<std::string> fields;
    std::vector<TupleEntry> tuples;
    std::vector<int> simpleAddrs;
};


struct FileHandle {
    std::string originalPath;
    std::string sandboxPath;
    bool isValid = true;
    int readLinePos = 0;
    int totalLines = 0;
    std::ifstream stream;
};

class AstraVM {
    typedef void (*OpFunc)(AstraVM* vm, const Instruction& instr);

    void executePowerCall(std::string funcName, int line);

     
    std::vector<Value> stack; 

    const std::vector<std::string>* stringPoolPtr = nullptr;
    const std::vector<double>* floatPoolPtr = nullptr;
    const std::vector<long long>* longPoolPtr = nullptr;

    

    int getVarId(const std::string& name) {
        if (vmSymbolTable.find(name) == vmSymbolTable.end()) {
            return allocSlot(name);
        }
        return vmSymbolTable[name];
    }

public:

     bool hasFatalMemoryError = false;
     bool hasDynamicResolution = false;

    int allocSlot(const std::string& name = "") {
        if (vmNextVarId >= MAX_MEMORY) {
            if (!hasFatalMemoryError) {
                AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                    "Memory limit exceeded (" + std::to_string(MAX_MEMORY) +
                    " variables/chain elements max). Program stopped.");
            }
            hasFatalMemoryError = true;
            return -1;
        }
        int addr = vmNextVarId++;
        if (!name.empty()) {
            vmSymbolTable[name] = addr;
            if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");
            varNames[addr] = name;
        }
        return addr;
    }

    std::unordered_map<std::string, ModifierEntry> modifierTable;


   std::vector<FileHandle> fileHandles;
   std::string sandboxDir; 

   std::string currentSelf = "";
   int currentLine = 0;

   
bool inWhenBlock     = false;
bool hasRuntimeError = false;
ErrCode lastErrCode  = ErrCode::NONE;
int whenEndAddress = 0; 
bool shouldJumpToWhenEnd = false; 
   
   int getVariableAddress(const std::string& name) {
    if (vmSymbolTable.find(name) != vmSymbolTable.end()) {
        return vmSymbolTable[name];
    }
    return -1; 
    }


    Value memory[MAX_MEMORY];
    void setStringPool(const std::vector<std::string>& pool) { stringPoolPtr = &pool; }
    void setFloatPool(const std::vector<double>& pool) { floatPoolPtr = &pool; }
    void setLongPool(const std::vector<long long>& pool) { longPoolPtr = &pool; }
    
    std::unordered_map<int, OpFunc> dispatchTable;
    void initDispatchTable();

    static void executePrint(AstraVM* vm, const Instruction& instr);
    static void executeArithmetic(AstraVM* vm, const Instruction& instr);

    AstraVM();
    ~AstraVM();
    
    void reset(); 

    std::vector<std::string> varNames;

    std::unordered_set<std::string> reportedErrors; 

    std::stack<CallFrame> callStack;
    std::unordered_map<std::string, FunctionEntry>* functionTable = nullptr;
    std::vector<Instruction> functionBytecode;

    
    std::unordered_map<std::string, ChainInfo> chainTable;

    void setVarNames(const std::vector<std::string>& names) { varNames = names; }
    std::string getVarName(int id) { 
        if (id >= 0 && id < (int)varNames.size()) return varNames[id];
        return "unknown";
    }

    void syncSymbolTable(const std::unordered_map<std::string, int>& symTable) {
        for (auto& pair : symTable) {
            vmSymbolTable[pair.first] = pair.second;
            if (pair.second >= vmNextVarId) vmNextVarId = pair.second + 1;
        }
    }
    std::unordered_map<std::string, int> vmSymbolTable;
    int vmNextVarId = 0;

    std::unordered_map<std::string, int> runtimeAliasTable;

     std::unordered_map<std::string, std::vector<std::string>> linkTable;   
    std::unordered_map<std::string, std::string> linkedFrom;               
    
    void push(Value v) { stack.push_back(v); }
    Value pop() { 
        if (stack.empty()) return {};
        Value v = stack.back(); 
        stack.pop_back(); 
        return v; 
    }
    Value& top() { return stack.back(); }
    Value peek();
    
    void execute(const std::vector<Instruction>& program);

    
    void chainStore(const std::string& chainName, 
                const std::vector<Value>& values,
                const std::vector<std::string>& fields = {});
    void chainPrint(const std::string& chainName);
    Value chainGet(const std::string& chainName, int index);
};

#endif