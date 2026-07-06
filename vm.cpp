/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "vm.h"
#include <iostream>
#include "common.h"
#include <algorithm>
#include "error.h"
#include "builtfun.h"
#include <sstream>
#include "file.h"
#include <functional>
#include <cmath>
#include <iomanip> 

 
ChainStoreFn g_chainStore = nullptr;

std::string formatFloat(double val) {
    std::string s = std::to_string(val);
    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    if (s.back() == '.') s.pop_back();
    return s;
}

AstraVM::AstraVM() { 
   
    reset(); 
    initDispatchTable(); 
}
AstraVM::~AstraVM() {}

void AstraVM::reset() {
    stack.clear();
    for(int i = 0; i < MAX_MEMORY; ++i) {
        memory[i].type = VAL_INT;
        memory[i].isInitialized = false;
        memory[i].num = 0;
    }
}

Value AstraVM::peek() {
    if (stack.empty()) { 
        std::cerr << "VM Error: Stack Underflow (peek)" << '\n';
        Value empty; 
        empty.type = VAL_INT; 
        empty.isInitialized = false; 
        return empty;
    }
    return stack.back(); 
}


void AstraVM::executePowerCall(std::string funcName, int line = 0)
{
    currentLine = line;
    auto& reg = PowerManager::getInstance().registry;

    if (reg.find(funcName) == reg.end()) {
        
        std::string displayName = funcName;
        size_t underscore = funcName.rfind('_');
        if (underscore != std::string::npos) {
            std::string suffix = funcName.substr(underscore + 1);
            bool allDigits = !suffix.empty() && 
                             std::all_of(suffix.begin(), suffix.end(), ::isdigit);
            if (allDigits) displayName = funcName.substr(0, underscore);
        }
        
        AstraError::runtime(ErrCode::FUNC_NOT_FOUND, line, displayName);
        return;
    }
    reg[funcName](this);
}

void AstraVM::executeArithmetic(AstraVM* vm, const Instruction& instr) {
    if (vm->stack.size()< 2) return;
    Value b = vm->pop(); 
    Value a = vm->pop(); 

     
    if (a.isPoisoned || b.isPoisoned) {
        Value poison;
        poison.isPoisoned = true;
        poison.isInitialized = false;
        poison.type = VAL_INT; 
        poison.str = a.isPoisoned ? a.str : b.str; 
        vm->push(poison);
        return;
    }

    if (a.type == VAL_BOOL || b.type == VAL_BOOL) {
    if (vm->inWhenBlock) {
        vm->hasRuntimeError = true;
        vm->lastErrCode = ErrCode::INVALID_OPERATION;
        vm->shouldJumpToWhenEnd = true;
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            "Cannot perform arithmetic on boolean");
    }
    Value poison;
    poison.isPoisoned = true;
    poison.type = VAL_INT;
    vm->push(poison);
    return;
    }
    
    Value res;
    res.isInitialized = true;

    
    if (instr.op == OP_ADD && (a.type == VAL_STR || b.type == VAL_STR)) {
        res.type = VAL_STR;
        std::string s1 = (a.type == VAL_STR) ? a.str : 
                         (a.type == VAL_FLOAT ? formatFloat(a.decimal) : std::to_string(a.num));
        std::string s2 = (b.type == VAL_STR) ? b.str : 
                         (b.type == VAL_FLOAT ? formatFloat(b.decimal) : std::to_string(b.num));
        res.str = s1 + s2;
    }
    
else if (instr.op != OP_ADD && (a.type == VAL_STR || b.type == VAL_STR)) {
    if (vm->inWhenBlock) {
        vm->hasRuntimeError = true;
        vm->lastErrCode = ErrCode::INVALID_OPERATION;
        vm->shouldJumpToWhenEnd = true;
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, 
            "Cannot perform arithmetic on string");
    }
    Value poison;
    poison.isPoisoned = true;
    poison.type = VAL_INT;
    vm->push(poison);
    return;
}
    
    else if ((instr.op == OP_DIV || instr.op == OP_MOD) && (b.type == VAL_INT ? b.num == 0 : b.decimal == 0.0)) {
        res.type = VAL_STR; 
        res.str = "INFINITE";
    } 
    
    else if (a.type == VAL_FLOAT || b.type == VAL_FLOAT) {
        res.type = VAL_FLOAT;
        double va = (a.type == VAL_FLOAT ? a.decimal : (double)a.num);
        double vb = (b.type == VAL_FLOAT ? b.decimal : (double)b.num);
        if (instr.op == OP_ADD) res.decimal = va + vb;
        else if (instr.op == OP_SUB) res.decimal = va - vb;
        else if (instr.op == OP_MUL) res.decimal = va * vb;
        else if (instr.op == OP_POW) res.decimal = std::pow(va, vb);
        else res.decimal = va / vb;
    } 
    
    else {
        res.type = VAL_INT;
        if (instr.op == OP_ADD) res.num = a.num + b.num;
        else if (instr.op == OP_SUB) res.num = a.num - b.num;
        else if (instr.op == OP_MUL) res.num = a.num * b.num;
        else if (instr.op == OP_DIV) res.num = a.num / b.num;
        else if (instr.op == OP_POW) res.num = std::pow(a.num, b.num);
        else res.num = a.num % b.num;
    }
    
    vm->push(res);
}

void AstraVM::executePrint(AstraVM* vm, const Instruction& instr) {
    if (vm->stack.size() < (size_t)instr.operand) return;
    
    int argCount = instr.operand;
    static std::vector<Value> args;
    args.clear();
    
    for(int i = 0; i < argCount; i++) {
        args.push_back(vm->pop());
    }

    
    bool allPoisoned = true;
    for (auto& a : args) {
        if (!a.isPoisoned) { allPoisoned = false; break; }
    }
    
if (allPoisoned) {
    for (auto& a : args) {
        if (a.isPoisoned && !a.str.empty()) {
            std::string errKey = a.str + ":" + std::to_string(instr.line);
            if (vm->reportedErrors.find(errKey) == vm->reportedErrors.end()) {
                
                if (!vm->inWhenBlock) {
                    AstraError::runtime(ErrCode::UNDEFINED_VAR, instr.line, a.str);
                }
                vm->reportedErrors.insert(errKey);
                
                if (vm->inWhenBlock) {
    if (!vm->hasRuntimeError) {
        vm->hasRuntimeError = true;
        vm->lastErrCode = ErrCode::UNDEFINED_VAR;
    }
    vm->shouldJumpToWhenEnd = true;
}
            }
            break;
        }
    }
    return;
}

    int repeatCount = 1; 
    bool shouldReverse = false; 
    bool isCommaSeparated = (argCount > 1);

    if (isCommaSeparated) {
        Value& lastVal = args[0];
        if (lastVal.type == VAL_INT) repeatCount = (int)lastVal.num;
        else if (lastVal.type == VAL_STR && lastVal.str == "rev") shouldReverse = true;
    }

    std::cout << "\033[32m";
    for (int r = 0; r < repeatCount; r++) {
        for (int i = args.size() - 1; i >= 0; i--) {
            if (isCommaSeparated && i == 0 && (args[0].type == VAL_INT || (args[0].type == VAL_STR && args[0].str == "rev"))) continue;
            
            Value& val = args[i];
            if (val.isPoisoned) {
                if (i > 0) std::cout << " ";
                continue;
            }
            if (val.type == VAL_INT) std::cout << val.num;
            else if (val.type == VAL_STR) std::cout << val.str;
            else if (val.type == VAL_PTR) std::cout << val.str;
            else if (val.type == VAL_FLOAT) std::cout << formatFloat(val.decimal);
            else if (val.type == VAL_BOOL) std::cout << (val.tristate == AST_TRUE ? "TRUE" : val.tristate == AST_MAYBE ? "MAYBE" : "FALSE"); 
            
            if (i > 0) std::cout << " ";
        }
        if (r < repeatCount - 1) std::cout << " ";
    }
    std::cout << "\033[0m" << '\n';
    //std::cout << '\n';
}

void AstraVM::initDispatchTable() {
    dispatchTable[(int)OP_PRINT] = &AstraVM::executePrint;
    dispatchTable[(int)OP_ADD] = &AstraVM::executeArithmetic;
    dispatchTable[(int)OP_SUB] = &AstraVM::executeArithmetic;
    dispatchTable[(int)OP_MUL] = &AstraVM::executeArithmetic;
    dispatchTable[(int)OP_DIV] = &AstraVM::executeArithmetic;
    dispatchTable[(int)OP_MOD] = &AstraVM::executeArithmetic;
    dispatchTable[(int)OP_POW] = &AstraVM::executeArithmetic;
}


static void chainStoreWrapper(void* vm,
                               const std::string& name,
                               std::vector<Value> values,
                               std::vector<std::string> fields) {
    ((AstraVM*)vm)->chainStore(name, values, fields);
}

static TriState toTriState(AstraVM* vm, const Value& v, int line) {
    if (v.type == VAL_BOOL) return v.tristate;
    if (v.type == VAL_INT) {
        if (v.num == 0) return AST_FALSE;
        if (v.num == 1) return AST_TRUE;
        AstraError::runtime(ErrCode::TYPE_MISMATCH, line,
            "expected a boolean value (0 or 1), got " + std::to_string(v.num));
        return AST_FALSE;
    }
    if (v.type == VAL_FLOAT) {
        if (v.decimal == 0.0) return AST_FALSE;
        if (v.decimal == 1.0) return AST_TRUE;
        AstraError::runtime(ErrCode::TYPE_MISMATCH, line,
            "expected a boolean value (0.0 or 1.0), got a non-boolean float");
        return AST_FALSE;
    }
    AstraError::runtime(ErrCode::TYPE_MISMATCH, line, "expected a boolean value (0 or 1)");
    return AST_FALSE;
}

void AstraVM::execute(const std::vector<Instruction>& program) {
    AstraError::setVM(this);
    reportedErrors.clear();

   g_chainStore = chainStoreWrapper; 
    
    stack.clear();
    reportedErrors.clear();
    int pc = 0; bool running = true;
    
    
    while (running && pc < (int)program.size()) {
        const Instruction& instr = program[pc++];

        if (dispatchTable.count(instr.op)) 
        { 
            dispatchTable[instr.op](this, instr); 
            
        if (shouldJumpToWhenEnd) {
            shouldJumpToWhenEnd = false;
            pc = whenEndAddress;
        }
            continue; 
        }

        switch (instr.op) {
            case OP_LOAD: { Value v; v.type = VAL_INT; v.num = instr.operand; v.isInitialized = true; push(v); break; }
            case OP_LOAD_STR: { Value v; v.type = VAL_STR; v.str = (*stringPoolPtr)[instr.operand]; v.isInitialized = true; push(v); break; }
            case OP_LOAD_FLOAT: { Value v; v.type = VAL_FLOAT; v.decimal = (*floatPoolPtr)[instr.operand]; push(v); break; }
            case OP_LOAD_LONG: { 
    Value v; 
    v.type = VAL_INT; 
    v.num = (*longPoolPtr)[instr.operand]; 
    v.isInitialized = true;
    push(v); 
    break; 
}
            case OP_LOAD_BOOL: { Value v; v.type = VAL_BOOL; v.tristate = static_cast<TriState>(instr.operand); push(v); break; }
            case OP_USER_INPUT: {
                std::cout << YELLOW << (*stringPoolPtr)[instr.operand] << RESET;
                std::string input; std::getline(std::cin, input);
                Value v; v.isInitialized = true;
                if (input == "true") { v.type = VAL_BOOL; v.tristate = AST_TRUE; }
                else if (input == "false") { v.type = VAL_BOOL; v.tristate = AST_FALSE; }
                else if (!input.empty() && (isdigit(input[0]) || input[0] == '-')) {
                    if (input.find('.') != std::string::npos) { v.type = VAL_FLOAT; v.decimal = std::stod(input); }
                    else { v.type = VAL_INT; v.num = std::stoll(input); }
                } else { v.type = VAL_STR; v.str = input; }
                push(v); break;
            }
            case OP_LOAD_VAR: {
     if (instr.operand == 9999) {
        if (!currentSelf.empty() && vmSymbolTable.count(currentSelf + ":" + instr.varName)) {
            int addr = vmSymbolTable[currentSelf + ":" + instr.varName];
            push(memory[addr]);
            break;
        }
        if (vmSymbolTable.count(instr.varName)) {
            int addr = vmSymbolTable[instr.varName];
            push(memory[addr]);
            break;
        }
        std::string errKey = instr.varName + ":" + std::to_string(instr.line);
        if (reportedErrors.find(errKey) == reportedErrors.end()) {
            reportedErrors.insert(errKey);
            
            if (inWhenBlock) {
                hasRuntimeError = true;
                lastErrCode     = ErrCode::UNDEFINED_VAR;
                shouldJumpToWhenEnd  = true;
            } else {
                AstraError::runtime(ErrCode::UNDEFINED_VAR, instr.line, instr.varName);
            }
        }
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        poison.str = instr.varName;
        push(poison);
        break;
    }

    Value val;
    if (instr.isLocal && !callStack.empty()) {
        int addr = callStack.top().basePointer + instr.operand;
        val = memory[addr];
    } else {
    int addr = instr.operand;
    if (hasDynamicResolution) {   
        if (!instr.varName.empty() && runtimeAliasTable.count(instr.varName)) {
            addr = runtimeAliasTable[instr.varName];
        } else if (!instr.varName.empty() && vmSymbolTable.count(instr.varName)) {
            addr = vmSymbolTable[instr.varName];
        }
    }
    val = memory[addr];
}

    
    if (val.isPoisoned) {
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        poison.str = val.str;
        push(poison);
        break;
    }

    
    if (!val.isInitialized) {
        AstraError::runtime(ErrCode::UNINITIALIZED_VAR, instr.line, getVarName(instr.operand)); 
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT;
        push(poison);
        break;
    }

    push(val);
    break;
}

case OP_STORE: {
    Value val = pop();
 
    if (val.isPoisoned) {
        int addr = instr.operand;
        if (instr.isLocal && !callStack.empty())
            addr = callStack.top().basePointer + instr.operand;
        if (memory[addr].isInitialized && !memory[addr].isPoisoned) {
            break;
        }
        memory[addr].isPoisoned = true;
        memory[addr].isInitialized = false;
        memory[addr].str = getVarName(addr);
        break;
    }

    int addr = instr.operand;
    if (instr.isLocal && !callStack.empty()) {
        addr = callStack.top().basePointer + instr.operand;
    } else if (hasDynamicResolution) {
        if (!instr.varName.empty() && runtimeAliasTable.count(instr.varName)) {
            addr = runtimeAliasTable[instr.varName];
        } else if (!instr.varName.empty() && vmSymbolTable.count(instr.varName)) {
            addr = vmSymbolTable[instr.varName];
        }
    }

    if (memory[addr].isInitialized && memory[addr].isConst) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line, "Cannot reassign constant variable");
        break; 
    }

    
    if (!instr.isLocal && !instr.varName.empty()) {
        if (hasDynamicResolution) {
            if (vmSymbolTable.count(instr.varName)) {
                addr = vmSymbolTable[instr.varName]; 
            } else {
                vmSymbolTable[instr.varName] = addr; 
            }
        } else if (!memory[addr].isInitialized) {
            
            vmSymbolTable[instr.varName] = addr;
            if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");
            varNames[addr] = instr.varName;
        }
    }

    if (!instr.isLocal && hasDynamicResolution) {
        std::string vname = (addr < (int)varNames.size()) ? varNames[addr] : "";
        if (vname.empty() && !instr.varName.empty()) {
            vname = instr.varName;
            if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");
            varNames[addr] = vname;
        }
        if (!vname.empty()) {
            vmSymbolTable[vname] = addr;
        }
    }
    
    memory[addr].type = val.type;
    memory[addr].isInitialized = true;
    memory[addr].isPoisoned = false; 
    
    if (instr.flag) {
        memory[addr].isConst = true;
    }

    if (val.type == VAL_STR || val.type == VAL_PTR) {
        memory[addr].str = val.str;
    }
    else if (val.type == VAL_FLOAT) memory[addr].decimal = val.decimal;
    else if (val.type == VAL_BOOL) memory[addr].tristate = val.tristate;
    else memory[addr].num = val.num;
    
    
    if (hasDynamicResolution) {
        if (!instr.varName.empty() && linkedFrom.count(instr.varName)) {
            std::string srcName = linkedFrom[instr.varName];
            auto& tgts = linkTable[srcName];
            tgts.erase(std::remove(tgts.begin(), tgts.end(), instr.varName), tgts.end());
            linkedFrom.erase(instr.varName);
        }
        
        if (!instr.varName.empty() && linkTable.count(instr.varName)) {
            for (auto& tgt : linkTable[instr.varName]) {
                if (vmSymbolTable.count(tgt)) {
                    int tgtAddr = vmSymbolTable[tgt];
                    memory[tgtAddr] = memory[addr];
                    memory[tgtAddr].isInitialized = true;
                }
            }
        }
    }
    
    break;
}
            case OP_EQ: case OP_NEQ: case OP_LT: case OP_GT: case OP_LTE: case OP_GTE: {
    Value b = pop(); Value a = pop(); Value res; res.type = VAL_BOOL; res.isInitialized = true;
    
    
    if (a.type == VAL_BOOL || b.type == VAL_BOOL) {
        bool result = (instr.op == OP_EQ)  ? (a.tristate == b.tristate) :
                      (instr.op == OP_NEQ) ? (a.tristate != b.tristate) : false;
        res.tristate = result ? AST_TRUE : AST_FALSE;
        push(res);
        break;
    }
    

    if (a.type == VAL_STR || b.type == VAL_STR) {
        std::string sa = (a.type == VAL_STR) ? a.str : std::to_string(a.num);
        std::string sb = (b.type == VAL_STR) ? b.str : std::to_string(b.num);
        bool result = (instr.op == OP_EQ)  ? (sa == sb) :
                      (instr.op == OP_NEQ) ? (sa != sb) :
                      (instr.op == OP_LT)  ? (sa <  sb) :
                      (instr.op == OP_GT)  ? (sa >  sb) :
                      (instr.op == OP_LTE) ? (sa <= sb) : (sa >= sb);
        res.tristate = result ? AST_TRUE : AST_FALSE;
        push(res);
        break;
    }
    
    
    double va = (a.type == VAL_FLOAT) ? a.decimal : (double)a.num;
    double vb = (b.type == VAL_FLOAT) ? b.decimal : (double)b.num;
    bool result = (instr.op == OP_EQ)  ? (va == vb) : (instr.op == OP_NEQ) ? (va != vb) :
                  (instr.op == OP_LT)  ? (va <  vb) : (instr.op == OP_GT)  ? (va >  vb) :
                  (instr.op == OP_LTE) ? (va <= vb) : (va >= vb);
    res.tristate = result ? AST_TRUE : AST_FALSE;
    push(res);
    break;
}
            case OP_JZ: { Value v = pop(); if (toTriState(this, v, instr.line) == AST_FALSE) pc = instr.operand; break; }
            case OP_JMP: pc = instr.operand; break;
          
case OP_LOOP_COND: {
    Value stepVal = pop();
    Value endVal = pop();
    Value iVal = pop();
    
    
    bool shouldStop;
    if (stepVal.num >= 0) {
        shouldStop = (iVal.num > endVal.num);
    } else {
        shouldStop = (iVal.num < endVal.num);
    }
    
    if (shouldStop) pc = instr.operand;
    break;
}


case OP_LOOP_STEP: {
    Value stepVal = pop();
    int addr = instr.operand;
    if (instr.isLocal && !callStack.empty())
        addr = callStack.top().basePointer + instr.operand;
    memory[addr].num += stepVal.num;
    break;
}
            case OP_LOOP_INC: memory[instr.operand].num++; break;
            case OP_INFO: {
    int varId = instr.operand;
    Value v = memory[varId];
    std::string vName = varNames[varId];

    std::cout << MAGENTA << "\n========================================" << RESET << '\n';
    std::cout << BOLD << "          Astra Variable Inspector      " << RESET << '\n';
    std::cout << MAGENTA << "========================================" << RESET << '\n';
    std::cout << CYAN << " Variable Name : " << RESET << vName << '\n';
    std::cout << CYAN << " Memory Index  : " << RESET << varId << '\n';
    std::cout << CYAN << " Initialized   : " << (v.isInitialized ? GREEN "YES" : RED "NO") << RESET << '\n';

    if (v.isInitialized) {
        std::cout << CYAN << " Data Type     : " << RESET << YELLOW;
        switch(v.type) {
            case VAL_INT:   std::cout << "Integer"; break;
            case VAL_FLOAT: std::cout << "Float"; break;
            case VAL_STR:   std::cout << "String"; break;
            case VAL_PTR:   std::cout << "Pointer"; break;
            case VAL_BOOL:  std::cout << "Boolean"; break;
        }
        std::cout << RESET << '\n';

        std::cout << CYAN << " Current Value : " << RESET;
        if (v.type == VAL_INT) std::cout << v.num;
        else if (v.type == VAL_FLOAT) std::cout << formatFloat(v.decimal);
        else if (v.type == VAL_STR) std::cout << "\"" << v.str << "\"";
        else if (v.type == VAL_PTR) {
    std::cout << "\"" << v.str << "\"";
    
   
    uintptr_t addr = std::stoull(v.str, nullptr, 16);
    Value* target = reinterpret_cast<Value*>(addr);
    
    if (target != nullptr) {
        std::cout << "\n " << CYAN << "Points to Value : " << RESET << target->num;
        
       
        std::cout << "\n " << CYAN << "Address of    : " << RESET;
        bool found = false;
        for (auto const& [name, address] : vmSymbolTable) {
            
            if (reinterpret_cast<uintptr_t>(&memory[address]) == addr) {
                std::cout << name;
                found = true;
                break;
            }
        }
        if (!found) std::cout << "Unknown Variable";
    }
}
        else if (v.type == VAL_BOOL) std::cout << (v.tristate == AST_TRUE ? "TRUE" : (v.tristate == AST_FALSE ? "FALSE" : "MAYBE"));
        std::cout << '\n';
    }
    std::cout << MAGENTA << "========================================\n" << RESET << '\n';
    break;
}
case OP_CHAIN_INFO: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    
    if (chainTable.find(chainName) == chainTable.end()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        " Chain '" + chainName + "' not found");
        break;
    }
    
    ChainInfo& info = chainTable[chainName];
    std::string base = chainName.substr(0, chainName.find(':'));
    
    std::cout << MAGENTA << "\n========================================" << RESET << '\n';
    std::cout << BOLD << "         Astra Chain Inspector          " << RESET << '\n';
    std::cout << MAGENTA << "========================================" << RESET << '\n';
    std::cout << CYAN << " Chain  : " << RESET << chainName << '\n';

    bool hasNamedEntries  = info.namedOffset > 0;
    bool hasSimpleEntries = !info.simpleAddrs.empty();

    // Named Objects
    if (hasNamedEntries) {
        std::cout << CYAN << " Fields : " << RESET;
        for (int i = 0; i < (int)info.fields.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << info.fields[i];
        }
        std::cout << '\n';
        std::cout << CYAN << " Objects: " << RESET << info.namedOffset << '\n';
        for (int i = 1; i <= info.namedOffset; i++) {
            std::cout << "  " << YELLOW << base << i << RESET << " : ";
            for (int f = 0; f < (int)info.fields.size(); f++) {
                std::string key = base + std::to_string(i) + ":" + info.fields[f];
                if (vmSymbolTable.count(key)) {
                    Value& v = memory[vmSymbolTable[key]];
                    std::cout << info.fields[f] << "=";
                    if (v.type == VAL_STR)   std::cout << "\"" << v.str << "\"";
                    else if (v.type == VAL_INT)   std::cout << v.num;
                    else if (v.type == VAL_FLOAT) std::cout << formatFloat(v.decimal);
                    if (f < (int)info.fields.size() - 1) std::cout << ", ";
                }
            }
            std::cout << '\n';
        }
    }

    // Simple Values
    if (hasSimpleEntries) {
        std::cout << CYAN << " Values : " << RESET << info.simpleAddrs.size() << '\n';
        for (int i = 0; i < (int)info.simpleAddrs.size(); i++) {
            Value& v = memory[info.simpleAddrs[i]];
            std::cout << "  " << YELLOW << base << (i + 1) << RESET << " : ";
            if (v.type == VAL_INT)        std::cout << v.num;
            else if (v.type == VAL_STR)   std::cout << "\"" << v.str << "\"";
            else if (v.type == VAL_FLOAT) std::cout << formatFloat(v.decimal);
            std::cout << '\n';
        }
    }

    if (!hasNamedEntries && !hasSimpleEntries) {
        std::cout << CYAN << " (empty chain)" << RESET << '\n';
    }

    std::cout << MAGENTA << "========================================\n" << RESET << '\n';
    break;
}

case OP_INFO_CMD: { 
    std::string cmd = (*stringPoolPtr)[instr.operand];
    
    
    const char* result = PowerManager::getInstance().queryInfo(cmd);
    if (result) {
        std::cout << MAGENTA << "\n========================================" << RESET << '\n';
        std::cout << BOLD << "       Astra Power Inspector" << RESET << '\n';
        std::cout << MAGENTA << "========================================" << RESET << '\n';
        std::cout << CYAN << " Power : " << RESET << cmd << '\n';
        std::cout << MAGENTA << "----------------------------------------" << RESET << '\n';
        
        std::string s = result;
        std::istringstream ss(s);
        std::string line;
        while (std::getline(ss, line)) {
            size_t pipe = line.find('|');
            if (pipe != std::string::npos) {
                std::string func = line.substr(0, pipe);
                std::string desc = line.substr(pipe + 1);
                std::cout << " " << YELLOW << std::left << std::setw(20) << func 
                          << RESET << ": " << desc << '\n';
            }
        }
        std::cout << MAGENTA << "========================================\n" << RESET << '\n';
        break; 
    }
    
    
std::cout << MAGENTA << "\n========================================" << RESET << '\n';
std::cout << BOLD << "       Astra Command Inspector" << RESET << '\n';
std::cout << MAGENTA << "========================================" << RESET << '\n';

if (cmd == "write") {
    std::cout << CYAN << " Command  : " << RESET << "write" << '\n';
    std::cout << MAGENTA << "----------------------------------------" << RESET << '\n';
    std::cout << BOLD << " Usage    : " << RESET << "write <expression>;" << '\n';
    std::cout << BOLD << " Function : " << RESET << "Prints the result to the console." << '\n';
} 
else if (cmd == "repeat") {
    std::cout << CYAN << " Command  : " << RESET << "repeat" << '\n';
    std::cout << MAGENTA << "----------------------------------------" << RESET << '\n';
    std::cout << BOLD << " Usage    : " << RESET << "repeat (i <= limit) { ... };" << '\n';
    std::cout << BOLD << " Function : " << RESET << "Executes a code block multiple times." << '\n';
}
else if (cmd == "if") {
    std::cout << CYAN << " Command  : " << RESET << "if" << '\n';
    std::cout << MAGENTA << "----------------------------------------" << RESET << '\n';
    std::cout << BOLD << " Usage    : " << RESET << "if (cond) { ... };" << '\n';
    std::cout << BOLD << " Function : " << RESET << "Conditional execution of code blocks." << '\n';
}
else if (cmd == "cls" || cmd == "clear") {
    std::cout << CYAN << " Command  : " << RESET << cmd << '\n';
    std::cout << MAGENTA << "----------------------------------------" << RESET << '\n';
    std::cout << BOLD << " Usage    : " << RESET << "cls;  OR  clear;" << '\n';
    std::cout << BOLD << " Function : " << RESET << "Clears the console screen." << '\n';
}
else {
    std::cout << RED << " Unknown command: " << cmd << RESET << '\n';
}

std::cout << MAGENTA << "========================================\n" << RESET << '\n';
break;
}
            case OP_CLS: {
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
                break;
            }
            case OP_POWER_CALL: {
    std::string funcName = (*stringPoolPtr)[instr.operand];
     
    executePowerCall(funcName, instr.line);
      
    break;
}case OP_NEG:
{
    Value v = pop();

    if (v.type == VAL_FLOAT) {
        v.decimal = -v.decimal;
    }
    else {
        v.num = -v.num;
    }

    push(v);
    break;
}
case OP_FUNC_CALL: {
    std::string funcName = (*stringPoolPtr)[instr.operand];

    
    if (callStack.size() > 5000) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        "Stack overflow : infinite recursion detected");
    
    while (!callStack.empty()) callStack.pop();
    running = false;  
    break;
}
    
    if (functionTable && functionTable->count(funcName)) {
        FunctionEntry& entry = (*functionTable)[funcName];
        
        CallFrame frame;
        frame.returnAddress = pc;
        frame.savedVmNextVarId = vmNextVarId;
        
        
        if (!callStack.empty()) {
            
            frame.basePointer = callStack.top().basePointer + 64;
        } else {
            frame.basePointer = vmNextVarId;
        }

       
if (frame.basePointer + 64 >= MAX_MEMORY) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        "Stack overflow : memory limit exceeded");
    while (!callStack.empty()) callStack.pop();
    running = false;
    break;
}
        
        int paramCount = entry.params.size();
        std::vector<Value> args(paramCount);
        for (int i = paramCount - 1; i >= 0; i--) args[i] = pop();
        for (int i = 0; i < paramCount; i++) {
            memory[frame.basePointer + entry.paramIds[i]] = args[i];
            memory[frame.basePointer + entry.paramIds[i]].isInitialized = true;
        }
        
        vmNextVarId = frame.basePointer + entry.localVarCount + 10;
        callStack.push(frame);
        pc = entry.startAddress;
    } else {
    
    std::string baseName = funcName.substr(0, funcName.rfind('_'));
    std::string givenStr = funcName.substr(funcName.rfind('_') + 1);
    int givenArgs = std::stoi(givenStr);
    
    bool foundWrongArgs = false;
    if (functionTable) {
        for (auto& [key, entry] : *functionTable) {
            std::string keyBase = key.substr(0, key.rfind('_'));
            if (keyBase == baseName) {
                foundWrongArgs = true;
                int expectedArgs = (int)entry.params.size();
                int givenArgsInt = std::stoi(givenStr);
                for (int i = 0; i < givenArgsInt; i++) pop();
                AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                    "'" + baseName + "' expects " + 
                    std::to_string(expectedArgs) + 
                    " args, got " + std::to_string(givenArgs));
                break;
            }
        }
    }
    
    if (!foundWrongArgs) {
        
        bool foundInAlias = false;
        if (functionTable) {
            for (auto& [key, entry] : *functionTable) {
                size_t dot = key.find('.');
                if (dot != std::string::npos) {
                    std::string withoutAlias = key.substr(dot + 1);
                    if (withoutAlias == funcName) {
                        foundInAlias = true;
                        std::string alias = key.substr(0, dot);
                        AstraError::runtime(ErrCode::ALIAS_REQUIRED, 0,
                            "'" + funcName.substr(0, funcName.rfind('_')) + 
                            "' — use '" + alias + "." + 
                            funcName.substr(0, funcName.rfind('_')) + "()'");
                        break;
                    }
                }
            }
        }
        if (!foundInAlias) {
            executePowerCall(funcName,instr.line);
        }
    }
}
    break;
}

case OP_MODIFIER_CALL: {
    std::string key = (*stringPoolPtr)[instr.operand];
    size_t underscore = key.rfind('_');
    std::string modName = key.substr(0, underscore);
    std::string section = key.substr(underscore + 1);
    
    if (modifierTable.count(modName)) {
        ModifierEntry& entry = modifierTable[modName];
        int jumpAddr = (section == "before") ? entry.beforeAddr : entry.afterAddr;
        
        if (jumpAddr >= 0) {
            CallFrame frame;
            frame.returnAddress    = pc;
            frame.savedVmNextVarId = vmNextVarId;
            frame.savedSelf        = currentSelf;
            
            if (!callStack.empty()) {
                frame.basePointer = callStack.top().basePointer + 64;
            } else {
                frame.basePointer = vmNextVarId;
            }
            
            if (frame.basePointer + 64 >= MAX_MEMORY) {
                AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                    "Stack overflow in modifier");
                running = false;
                break;
            }
            
            vmNextVarId = frame.basePointer + 64;
            callStack.push(frame);
            pc = jumpAddr;
        }
    }
    break;
}
case OP_RETURN_VAL: {
    Value retVal = pop();
    
    if (!callStack.empty()) {
        CallFrame frame = callStack.top();
        callStack.pop();
        vmNextVarId = frame.savedVmNextVarId; 
        pc = frame.returnAddress;
    }
    
    push(retVal);
    break;
}
case OP_RETURN: {
    if (!callStack.empty()) {
        CallFrame frame = callStack.top();
        callStack.pop();
        vmNextVarId = frame.savedVmNextVarId; 
        currentSelf = frame.savedSelf; 
        pc = frame.returnAddress;
    }
    break;
}

case OP_METHOD_CALL: {
    std::string full = (*stringPoolPtr)[instr.operand]; 
    
    
    size_t colon = full.find(':');
    std::string objName  = full.substr(0, colon); 
    std::string methName = full.substr(colon + 1); 
    
    
    std::string base = objName;
    while (!base.empty() && isdigit(base.back())) 
        base.pop_back(); 
    
   
    std::string chainKey = "";
    
    for (auto& [k, v] : chainTable) {
        std::string kb = k.substr(0, k.find(':'));
       
        if (kb == base) { chainKey = k; break; } 
    }
    
    
    std::string funcKey = base + ":n:" + methName;
    
    
    if (functionTable && functionTable->count(funcKey)) {
        FunctionEntry& entry = (*functionTable)[funcKey];
        
        
        std::string savedSelf = currentSelf;
        currentSelf = objName; 
        
        CallFrame frame;
        frame.returnAddress    = pc;
        frame.savedVmNextVarId = vmNextVarId;
        
        if (!callStack.empty())
            frame.basePointer = callStack.top().basePointer + 64;
        else
            frame.basePointer = vmNextVarId;
        
        int paramCount = entry.params.size();
        std::vector<Value> args(paramCount);
        for (int i = paramCount - 1; i >= 0; i--) args[i] = pop();
        for (int i = 0; i < paramCount; i++) {
            memory[frame.basePointer + entry.paramIds[i]] = args[i];
            memory[frame.basePointer + entry.paramIds[i]].isInitialized = true;
        }
        
        vmNextVarId = frame.basePointer + paramCount;
        callStack.push(frame);
        pc = entry.startAddress;
        
        
        frame.savedSelf = savedSelf; 
    } else {
        std::cerr << RED << "[ASTRA-RUN-ERR]" << RESET
                  << " :: Method '" << funcKey << "' not found." << '\n';
    }
    break;
}
case OP_CHAIN_STORE: {
    std::string full = (*stringPoolPtr)[instr.operand];
    
    size_t pipe = full.find('|');
    std::string chainName = full.substr(0, pipe);
    std::string fieldStr = full.substr(pipe + 1);

    
    std::vector<std::string> fields;
    if (!fieldStr.empty()) {
        std::string f;
        for (char c : fieldStr) {
            if (c == ',') { fields.push_back(f); f = ""; }
            else f += c;
        }
        if (!f.empty()) fields.push_back(f);
    }

    int count = (int)pop().num;
    std::vector<Value> values(count);
    for (int i = count - 1; i >= 0; i--) {
        values[i] = pop();
    }

for (auto& v : values)
     
    if (count == 0) {
        if (chainTable.find(chainName) == chainTable.end()) {
            ChainInfo info;
            info.startId     = vmNextVarId;
            info.count       = 0;
            info.totalSlots  = 0;
            info.namedOffset = 0;
            info.fields      = fields;
            chainTable[chainName] = info;
        }
        break;
    }
    chainStore(chainName, values, fields);
    break;
}
case OP_CHAIN_PRINT: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    chainPrint(chainName);
    break;
}
case OP_CHAIN_FIELD_LOAD: {
    
    std::string full = (*stringPoolPtr)[instr.operand];
    size_t colon = full.find(':');
    std::string varPart = full.substr(0, colon);   
    std::string field = full.substr(colon + 1);     
    
    
    std::string key = varPart + ":" + field;
    if (vmSymbolTable.count(key)) {
        int addr = vmSymbolTable[key];
        push(memory[addr]);
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            "Field '" + key + "' not found");
        Value poison;
        poison.isPoisoned = true;
        poison.type = VAL_INT;
        push(poison);   
    }
    break;
}
case OP_CHAIN_FIELD_STORE: {
    Value val = pop();
    std::string full = (*stringPoolPtr)[instr.operand];
    
    
    if (vmSymbolTable.count(full)) {
        int addr = vmSymbolTable[full];
        memory[addr] = val;
        memory[addr].isInitialized = true;
    } else {
        
        int addr = allocSlot(full);
        if (addr == -1) { running = false; break; }
        vmSymbolTable[full] = addr;
        memory[addr] = val;
        memory[addr].isInitialized = true;
        varNames.push_back(full);
    }
    break;
}
case OP_CHAIN_DYNAMIC_LOAD: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    Value iterVal = pop();
    int index = (int)iterVal.num;
    std::string key = chainName + std::to_string(index);
    if (vmSymbolTable.count(key)) {
        push(memory[vmSymbolTable[key]]);
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            "Index '" + std::to_string(index) + "' out of bounds for chain '" + chainName + "'");
        Value poison;
        poison.isPoisoned = true;
        poison.type = VAL_INT;
        push(poison);
    }
    break;
}

case OP_CHAIN_DYNAMIC_STORE: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    Value iterVal = pop(); 
    Value val     = pop(); 
    int index = (int)iterVal.num;
    std::string key = chainName + std::to_string(index);

    if (vmSymbolTable.count(key)) {
        
        memory[vmSymbolTable[key]] = val;
        memory[vmSymbolTable[key]].isInitialized = true;
    } else {
        
        int addr = allocSlot(key);
        if (addr == -1) { running = false; break; }
        memory[addr] = val;
        memory[addr].isInitialized = true;

       
        std::string base = chainName.substr(0, chainName.find(':'));
        if (chainTable.count(chainName)) {
            ChainInfo& info = chainTable[chainName];
            TupleEntry entry;
            entry.startAddr  = addr;
            entry.fieldCount = 1;
            entry.hasFields  = false;
            info.tuples.push_back(entry);
            info.simpleAddrs.push_back(addr);
            info.totalSlots++;
            info.count++;
        }
    }
    break;
}
case OP_CHAIN_DYNAMIC_FIELD_STORE: {
    std::string full = (*stringPoolPtr)[instr.operand];
    size_t pipe = full.find('|');
    std::string chainName = full.substr(0, pipe); 
    std::string fieldName = full.substr(pipe + 1); 
    
    Value iterVal = pop(); 
    Value val     = pop(); 
    int index = (int)iterVal.num;
    
    
    std::string base = chainName.substr(0, chainName.find(':'));
    std::string key = base + std::to_string(index) + ":" + fieldName;
    
    if (vmSymbolTable.count(key)) {
        memory[vmSymbolTable[key]] = val;
        memory[vmSymbolTable[key]].isInitialized = true;
    } else {
        
        int addr = allocSlot(key);
        if (addr == -1) { running = false; break; }
        vmSymbolTable[key] = addr;
        if (addr >= (int)varNames.size()) varNames.resize(addr + 1, "");
        varNames[addr] = key;
        memory[addr] = val;
        memory[addr].isInitialized = true;
    }
    break;
}case OP_CHAIN_DYNAMIC_FIELD_LOAD: {
    std::string full = (*stringPoolPtr)[instr.operand];
    size_t pipe = full.find('|');
    std::string chainName = full.substr(0, pipe); 
    std::string fieldName = full.substr(pipe + 1); 
    
    Value iterVal = pop(); 
    int index = (int)iterVal.num;
    
    std::string base = chainName.substr(0, chainName.find(':'));
    std::string key = base + std::to_string(index) + ":" + fieldName; 
    
    if (vmSymbolTable.count(key)) {
        push(memory[vmSymbolTable[key]]);
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            " Field '" + key + "' not found");
        Value poison;
        poison.isPoisoned = true;
        poison.type = VAL_INT;
        push(poison);
    }
    break;
}
case OP_CHAIN_DYNAMIC_METHOD_CALL: {
    std::string full = (*stringPoolPtr)[instr.operand];
    size_t pipe = full.find('|');
    std::string chainBase = full.substr(0, pipe); 
    std::string methName  = full.substr(pipe + 1); 
    
    Value iterVal = pop(); 
    int index = (int)iterVal.num;
    
    
    std::string objName = chainBase + std::to_string(index);
    
    
    std::string funcKey = chainBase + ":n:" + methName;
    
    if (functionTable && functionTable->count(funcKey)) {
        FunctionEntry& entry = (*functionTable)[funcKey];
        
        std::string savedSelf = currentSelf;
        currentSelf = objName;
        
        CallFrame frame;
        frame.returnAddress    = pc;
        frame.savedVmNextVarId = vmNextVarId;
        frame.savedSelf        = savedSelf;
        
        if (!callStack.empty())
            frame.basePointer = callStack.top().basePointer + 64;
        else
            frame.basePointer = vmNextVarId;
        
        int paramCount = entry.params.size();
        std::vector<Value> args(paramCount);
        for (int i = paramCount - 1; i >= 0; i--) args[i] = pop();
        for (int i = 0; i < paramCount; i++) {
            memory[frame.basePointer + entry.paramIds[i]] = args[i];
            memory[frame.basePointer + entry.paramIds[i]].isInitialized = true;
        }
        
        vmNextVarId = frame.basePointer + paramCount;
        callStack.push(frame);
        pc = entry.startAddress;
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        " Method '" + funcKey + "' not found");
    }
    break;
}
case OP_CHAIN_SELF: {
    std::string full = (*stringPoolPtr)[instr.operand];
    std::string chainName = full.substr(5); 
    
    int count = (int)pop().num;
    std::vector<Value> values(count);
    for (int i = count - 1; i >= 0; i--) values[i] = pop();
    
    if (chainTable.count(chainName)) {
        std::vector<std::string> fields = chainTable[chainName].fields;
        chainStore(chainName, values, fields);
    } else {
         AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        " Chain '" + chainName + "' not defined");
    }
    break;
}
case OP_CHAIN_LEN: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainLen(this, chainName);
    break;
}
case OP_CHAIN_SORT: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainSort(this, chainName);
    break;
}
case OP_CHAIN_MERGE: {
    std::string full = (*stringPoolPtr)[instr.operand];
    size_t pipe = full.find('|');
    std::string c1 = full.substr(0, pipe);
    std::string c2 = full.substr(pipe + 1);
    BuiltinFunctions::chainMerge(this, c1, c2);
    break;
}
case OP_CHAIN_UNIQUE: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainUnique(this, chainName);
    break;
}
case OP_CHAIN_SUM: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainSum(this, chainName);
    break;
}
case OP_CHAIN_AVG: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainAvg(this, chainName);
    break;
}
case OP_CHAIN_MAX: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainMax(this, chainName);
    break;
}
case OP_CHAIN_MIN: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainMin(this, chainName);
    break;
}
case OP_CHAIN_REVERSE: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    BuiltinFunctions::chainReverse(this, chainName);
    break;
}
case OP_CHAIN_CONTAINS: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    Value target = pop();
    BuiltinFunctions::chainContains(this, chainName, target);
    break;
}
case OP_CHAIN_INDEXOF: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    Value target = pop();
    BuiltinFunctions::chainIndexOf(this, chainName, target);
    break;
}
case OP_CHAIN_JOIN: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    Value delimVal = pop();
    std::string delim = (delimVal.type == VAL_STR) ? delimVal.str : ",";
    BuiltinFunctions::chainJoin(this, chainName, delim);
    break;
}

case OP_AND: {
    Value b = pop(); 
    Value a = pop();
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    TriState ta = toTriState(this, a, instr.line);
    TriState tb = toTriState(this, b, instr.line);
    res.tristate = (ta == AST_TRUE && tb == AST_TRUE) ? AST_TRUE : AST_FALSE;
    push(res); 
    break;
}

case OP_OR: {
    Value b = pop(); 
    Value a = pop();
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    TriState ta = toTriState(this, a, instr.line);
    TriState tb = toTriState(this, b, instr.line);
    res.tristate = (ta == AST_TRUE || tb == AST_TRUE) ? AST_TRUE : AST_FALSE;
    push(res); 
    break;
}

case OP_NOT: {
    Value v = pop();
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    res.tristate = (toTriState(this, v, instr.line) == AST_TRUE) ? AST_FALSE : AST_TRUE;
    push(res); 
    break;
}
case OP_BREAK: {
    pc = instr.operand; 
    break;
}
case OP_CONTINUE: {
    pc = instr.operand; 
    break;
}

case OP_CALL_ADR: {
    int addr = instr.operand;
    if (instr.isLocal && !callStack.empty())
        addr = callStack.top().basePointer + instr.operand;

    if (instr.flag) {
        
        Value newVal = pop();

        Value& ptr = memory[addr];
        if (ptr.type != VAL_PTR) {
            AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
                        " target is not a pointer");
            break;
        }

        uintptr_t rawAddress = std::stoull(ptr.str, nullptr, 16);
        Value* target = reinterpret_cast<Value*>(rawAddress);
        if (target != nullptr) {
            *target = newVal;
            target->isInitialized = true;
            target->isPoisoned = false;
        }
    } else {
        
        uintptr_t rawAddress = reinterpret_cast<uintptr_t>(&memory[addr]);
        std::stringstream ss;
        ss << "0x" << std::hex << rawAddress;

        Value v;
        v.type = VAL_PTR;
        v.str = ss.str();
        v.isInitialized = true;
        push(v);
    }
    break;
}


case OP_CALL_VAL: {
    Value addrVal = pop();
    BuiltinFunctions::val(this, addrVal); 
    break;
}
case OP_CHECK_INT: {
    Value v = peek(); 
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    res.tristate = (v.type == VAL_INT && v.num == instr.operand) ? AST_TRUE : AST_FALSE;
    push(res);
    break;
}
case OP_CHECK_STR: {
    Value v = peek();
    std::string s = (*stringPoolPtr)[instr.operand];
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    res.tristate = (v.type == VAL_STR && v.str == s) ? AST_TRUE : AST_FALSE;
    push(res);
    break;
}
case OP_CHECK_RANGE: {
    Value v = peek();
    int start = instr.operand;
    int end   = instr.line; 
    Value res; res.type = VAL_BOOL; res.isInitialized = true;
    res.tristate = (v.type == VAL_INT && v.num >= start && v.num <= end) ? AST_TRUE : AST_FALSE;
    push(res);
    break;
}
case OP_CHECK_END: {
    pop(); 
    break;
}
case OP_FILE_CREATE: {
    Value pathVal = pop();
    if (pathVal.type != VAL_STR) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        "create() expects a string path");
        Value poison;
        poison.isPoisoned = true;
        poison.type = VAL_INT;
        push(poison);
        break;
    }
    int handleId = FileOps::create(this, pathVal.str);

    Value v;
    v.type = VAL_FILE;
    v.num = handleId;
    v.isInitialized = true;
    push(v);
    break;
}

case OP_FILE_CLOSE: {
    Value handleVal = pop();
    if (handleVal.type != VAL_FILE) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line," close() expects a file handle");
        break;
    }
    FileOps::close(this, (int)handleVal.num);
    break;
}
case OP_FILE_PLUS: {
    Value modeVal = pop();
    Value textVal = pop();
    Value handleVal = pop();

    if (handleVal.type != VAL_FILE) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            " plus() expects a file handle");
        break;
    }
    if (modeVal.type != VAL_STR) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            " plus() mode must be a string");
        break;
    }

    std::string text = valueToString(textVal);
    FileOps::plus(this, (int)handleVal.num, text, modeVal.str);
    break;
}
case OP_FILE_READ: {
    Value modeVal = pop();
    Value handleVal = pop();

    if (handleVal.type != VAL_FILE) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            " read() expects a file handle");
        push({});
        break;
    }
    if (modeVal.type != VAL_STR) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            " read() mode must be a string");
        push({});
        break;
    }

    Value result;
    if (modeVal.str == "l") {
        result = FileOps::readLine(this, (int)handleVal.num);
    } else if (modeVal.str == "t") {
        result = FileOps::readAll(this, (int)handleVal.num);
    } else {
       AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
        "read() invalid mode: '" + modeVal.str + "'");
        result.type = VAL_STR;
        result.str = "";
        result.isInitialized = true;
    }

    push(result);
    break;
}
case OP_FILE_EOF: {
    Value handleVal = pop();
    if (handleVal.type != VAL_FILE) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line, "eof() expects file handle");
        push({});
        break;
    }
    bool isEof = FileOps::isEof(this, (int)handleVal.num);
    Value v;
    v.type = VAL_BOOL;
    v.tristate = isEof ? AST_TRUE : AST_FALSE;
    v.isInitialized = true;
    push(v);
    break;
}
case OP_FILE_FETCH: {
    Value modeVal   = pop(); 
    Value pos2Val   = pop(); 
    Value pos1Val   = pop(); 
    Value handleVal = pop(); 

    if (handleVal.type != VAL_FILE) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line, "fetch() expects file handle");
        push({});
        break;
    }
    
    Value result = FileOps::fetch(this, (int)handleVal.num,
                                  modeVal.str,
                                  (int)pos1Val.num,
                                  (int)pos2Val.num);
    push(result);
    break;
}
case OP_JSON_PARSE: {
    Value jsonVal      = pop();  
    Value chainNameVal = pop();  

    if (jsonVal.type != VAL_STR || chainNameVal.type != VAL_STR) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            "parseJson() expects (chainName, jsonString) strings");
        break;
    }

    std::vector<std::string> fields;
    std::vector<Value> values;
    const std::string& s = jsonVal.str;

    std::function<void(size_t&)> skipSpaces = [&](size_t& i) {
        while (i < s.size() && isspace((unsigned char)s[i])) i++;
    };
    std::function<std::string(size_t&)> parseStr = [&](size_t& i) -> std::string {
        std::string out;
        i++; 
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) { out += s[i+1]; i += 2; }
            else { out += s[i]; i++; }
        }
        i++; 
        return out;
    };
    std::function<Value(size_t&)> parseVal = [&](size_t& i) -> Value {
        skipSpaces(i);
        Value v; v.isInitialized = true;
        if (s[i] == '"') {
            v.type = VAL_STR; v.str = parseStr(i);
        } else if (s[i] == 't' || s[i] == 'f') {
            bool isTrue = (s.compare(i, 4, "true") == 0);
            v.type = VAL_BOOL; v.tristate = isTrue ? AST_TRUE : AST_FALSE;
            i += isTrue ? 4 : 5;
        } else if (s[i] == 'n') {
            v.type = VAL_STR; v.str = ""; i += 4;
        } else {
            size_t start = i; bool isFloat = false;
            if (s[i] == '-') i++;
            while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '.')) {
                if (s[i] == '.') isFloat = true;
                i++;
            }
            std::string numStr = s.substr(start, i - start);
            if (isFloat) { v.type = VAL_FLOAT; v.decimal = std::stod(numStr); }
            else         { v.type = VAL_INT;   v.num     = std::stoll(numStr); }
        }
        return v;
    };
    std::function<void(size_t&, const std::string&)> parseObj = [&](size_t& i, const std::string& prefix) {
        skipSpaces(i);
        if (i >= s.size() || s[i] != '{') return;
        i++; skipSpaces(i);
        while (i < s.size() && s[i] != '}') {
            skipSpaces(i);
            std::string key = parseStr(i);
            skipSpaces(i);
            if (i < s.size() && s[i] == ':') i++;
            skipSpaces(i);
            std::string fullKey = prefix.empty() ? key : prefix + ":" + key;
            if (i < s.size() && s[i] == '{') {
                parseObj(i, fullKey);
            } else {
                Value v = parseVal(i);
                fields.push_back(fullKey);
                values.push_back(v);
            }
            skipSpaces(i);
            if (i < s.size() && s[i] == ',') { i++; skipSpaces(i); }
        }
        if (i < s.size() && s[i] == '}') i++;
    };

    size_t pos0 = 0;
skipSpaces(pos0);

if (pos0 < s.size() && s[pos0] == '[') {
    
    pos0++; 
    skipSpaces(pos0);
    while (pos0 < s.size() && s[pos0] != ']') {
        fields.clear();
        values.clear();
        parseObj(pos0, "");
        chainStore(chainNameVal.str, values, fields);  
        skipSpaces(pos0);
        if (pos0 < s.size() && s[pos0] == ',') { pos0++; skipSpaces(pos0); }
    }
} else {
    
    parseObj(pos0, "");
    chainStore(chainNameVal.str, values, fields);
}
    break;
}
case OP_TO_JSON: {
    std::string chainName = (*stringPoolPtr)[instr.operand]; 
    std::string base = chainName.substr(0, chainName.find(':'));

    if (chainTable.find(chainName) == chainTable.end()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
            "toJson(): chain '" + chainName + "' not found");
        Value v; v.type = VAL_STR; v.str = ""; v.isInitialized = true;
        push(v);
        break;
    }

    ChainInfo& info = chainTable[chainName];
    bool hasFields = !info.fields.empty();

   
    std::function<std::string(const Value&)> valToJson = [&](const Value& v) -> std::string {
        if (v.type == VAL_STR) {
            std::string esc;
            for (char c : v.str) {
                if (c == '"' || c == '\\') esc += '\\';
                esc += c;
            }
            return "\"" + esc + "\"";
        }
        if (v.type == VAL_INT)   return std::to_string(v.num);
        if (v.type == VAL_FLOAT) return formatFloat(v.decimal);
        if (v.type == VAL_BOOL)  return (v.tristate == AST_TRUE) ? "true" : "false";
        return "null";
    };

    
    std::function<std::string(int)> buildObject = [&](int objIndex) -> std::string {
        
        std::vector<std::string> topKeys;
        std::map<std::string, std::vector<std::pair<std::string, Value>>> nestedMap;
        std::map<std::string, Value> flatMap;

        for (auto& f : info.fields) {
            std::string key = base + std::to_string(objIndex) + ":" + f;
            if (!vmSymbolTable.count(key)) continue;
            Value v = memory[vmSymbolTable[key]];

            size_t colon = f.find(':');
            if (colon == std::string::npos) {
                if (std::find(topKeys.begin(), topKeys.end(), f) == topKeys.end())
                    topKeys.push_back(f);
                flatMap[f] = v;
            } else {
                std::string topKey = f.substr(0, colon);
                std::string subKey = f.substr(colon + 1);
                if (std::find(topKeys.begin(), topKeys.end(), topKey) == topKeys.end())
                    topKeys.push_back(topKey);
                nestedMap[topKey].push_back({subKey, v});
            }
        }

        std::string out = "{";
        for (size_t i = 0; i < topKeys.size(); i++) {
            if (i > 0) out += ",";
            const std::string& k = topKeys[i];
            out += "\"" + k + "\":";
            if (nestedMap.count(k)) {
                out += "{";
                auto& subs = nestedMap[k];
                for (size_t j = 0; j < subs.size(); j++) {
                    if (j > 0) out += ",";
                    out += "\"" + subs[j].first + "\":" + valToJson(subs[j].second);
                }
                out += "}";
            } else {
                out += valToJson(flatMap[k]);
            }
        }
        out += "}";
        return out;
    };

    std::string result;

    if (hasFields) {
        int objCount = info.namedOffset;
        if (objCount <= 1) {
            result = (objCount == 1) ? buildObject(1) : "{}";
        } else {
            result = "[";
            for (int i = 1; i <= objCount; i++) {
                if (i > 1) result += ",";
                result += buildObject(i);
            }
            result += "]";
        }
    } else {
        
        result = "[";
        for (size_t i = 0; i < info.simpleAddrs.size(); i++) {
            if (i > 0) result += ",";
            result += valToJson(memory[info.simpleAddrs[i]]);
        }
        result += "]";
    }

    Value v;
    v.type = VAL_STR;
    v.str  = result;
    v.isInitialized = true;
    push(v);
    break;
}
case OP_JSON_PRETTY: {
    Value jsonVal = pop();
    if (jsonVal.type != VAL_STR) {
        AstraError::runtime(ErrCode::TYPE_MISMATCH, instr.line,
            "jsonpretty() expects a string argument");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR;
        push(poison);
        break;
    }
    BuiltinFunctions::jsonPretty(this, jsonVal.str);
    break;
}
case OP_FILE_CLEAR: {
    if (instr.operand == 0) {
        
        Value handleVal = pop();
        if (handleVal.type != VAL_FILE) {
            AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
                " clear() expects a file handle");
            break;
        }
        FileOps::clear(this, (int)handleVal.num);
    }
    else {
        
        Value textVal   = pop();
        Value handleVal = pop();
        if (handleVal.type != VAL_FILE) {
             AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
                " clear() expects a file handle");
            break;
        }
        FileOps::clearLine(this, (int)handleVal.num, valueToString(textVal));
    }
    break;
}case OP_WRITES: {
    int argCount = instr.operand; 
    std::vector<Value> args(argCount);
    
    
    for(int i = argCount - 1; i >= 0; i--) {
        args[i] = pop();
    }
    
    
    std::cout << "\033[32m"; 
    for(int i = 0; i < argCount; i++) {
        Value& v = args[i];
        
        
        if (v.type == VAL_INT) std::cout << v.num;
        else if (v.type == VAL_STR) std::cout << v.str;
        else if (v.type == VAL_FLOAT) std::cout << formatFloat(v.decimal);
        else if (v.type == VAL_BOOL) std::cout << (v.tristate == AST_TRUE ? "TRUE" : "FALSE");
        
        
        if (i < argCount - 1) std::cout << "";
    }
    std::cout << "\033[0m" << '\n'; 
    break;
}

case OP_WHEN_START: {
    inWhenBlock     = true;
    hasRuntimeError = false;
    lastErrCode     = ErrCode::NONE;
    whenEndAddress  = instr.operand; 
    break;
}

case OP_WHEN_END: {
    inWhenBlock = false;
    if (!hasRuntimeError) {
        
        pc = instr.operand;
    }
    break;
}

case OP_THEN_CHECK: {
    ErrCode expected = (ErrCode)instr.operand;
    if (expected == ErrCode::NONE) {
        
        if (!hasRuntimeError) {
            pc = instr.line; 
        }
        break;
    }
    if (lastErrCode != expected) {
        pc = instr.line; 
        break;
    }
    
    hasRuntimeError = false;
    lastErrCode = ErrCode::NONE;
    break;
}

case OP_THEN_END: {
    hasRuntimeError = false;
    lastErrCode     = ErrCode::NONE;
    pc = instr.operand; 
    break;
}
case OP_ALIAS: {
    std::string aliasName  = (*stringPoolPtr)[instr.operand];
    std::string targetName = (*stringPoolPtr)[instr.operand + 1];
    
    if (vmSymbolTable.count(targetName)) {
        runtimeAliasTable[aliasName] = vmSymbolTable[targetName];
    }
    break;
}

case OP_DEALIAS: {
    std::string aliasName = (*stringPoolPtr)[instr.operand];
    
    int oldAddr = -1;
    if (runtimeAliasTable.count(aliasName)) {
        oldAddr = runtimeAliasTable[aliasName];
    } else if (vmSymbolTable.count(aliasName)) {
        oldAddr = vmSymbolTable[aliasName];
    }
    
    int newAddr = vmNextVarId;
    if (newAddr >= MAX_MEMORY) {
        if (!hasFatalMemoryError) {
            AstraError::runtime(ErrCode::INVALID_OPERATION, instr.line,
                "Memory limit exceeded (" + std::to_string(MAX_MEMORY) +
                " variables/chain elements max). Program stopped.");
        }
        hasFatalMemoryError = true;
        running = false;
        break;
    }
    vmNextVarId++;

    if (oldAddr >= 0) {
        memory[newAddr] = memory[oldAddr]; 
    }
    memory[newAddr].isInitialized = true;
    
    vmSymbolTable[aliasName] = newAddr;
    if (newAddr >= (int)varNames.size()) varNames.resize(newAddr + 1, "");
    varNames[newAddr] = aliasName;
    
    runtimeAliasTable.erase(aliasName);
    break;
}
case OP_LINK: {
    std::string source = (*stringPoolPtr)[instr.operand];
    std::string targetsStr = (*stringPoolPtr)[instr.operand + 1];

    std::vector<std::string> targets;
    std::string cur;
    for (char c : targetsStr) {
        if (c == ',') { targets.push_back(cur); cur = ""; }
        else cur += c;
    }
    if (!cur.empty()) targets.push_back(cur);

    for (auto& tgt : targets) {
        
        auto& srcTargets = linkTable[source];
        if (std::find(srcTargets.begin(), srcTargets.end(), tgt) == srcTargets.end()) {
            srcTargets.push_back(tgt);
        }
        linkedFrom[tgt] = source;

        
        if (vmSymbolTable.count(source) && vmSymbolTable.count(tgt)) {
            int srcAddr = vmSymbolTable[source];
            int tgtAddr = vmSymbolTable[tgt];
            if (memory[srcAddr].isInitialized) {
                memory[tgtAddr] = memory[srcAddr];
                memory[tgtAddr].isInitialized = true;
            }
        }
    }
    break;
}

case OP_HINT: {
    if (!stack.empty()) {
        Value& v = stack.back();
        if (v.isInitialized && !v.isPoisoned) {
            std::cout << MAGENTA << "[ASTRA-INFO]" << RESET
                      << " :: " << YELLOW << "Line " << instr.line << RESET
                      << " :: Use " << CYAN << "'write " << instr.varName << "'" << RESET
                      << " to print the value." << '\n';
        }
    }
    break;
}
case OP_EXE_CHAIN: {
    std::string chainName = (*stringPoolPtr)[instr.operand];
    std::string chainBase = (*stringPoolPtr)[instr.operand + 1];
    int exprAddr = instr.line;
    bool hasFilter = instr.flag;
    int condAddr = instr.outerDepth;

    if (!chainTable.count(chainName)) {
        AstraError::runtime(ErrCode::CHAIN_NOT_FOUND, 0, chainName);
        break;
    }
    ChainInfo& chain = chainTable[chainName];
    bool isNamed = !chain.fields.empty();   
    std::string base = chainName.substr(0, chainName.find(':'));

    int indexAddr;
    if (vmSymbolTable.count("i")) indexAddr = vmSymbolTable["i"];
    else {
    indexAddr = allocSlot("i");
    if (indexAddr == -1) { running = false; break; }
    }
    Value savedIndexValue = memory[indexAddr];

    
    std::vector<int> fieldTempAddrs;
    std::vector<Value> savedFieldValues;
    if (isNamed) {
        for (auto& f : chain.fields) {
            std::string key = chainBase + ":" + f;
            int addr;
            if (vmSymbolTable.count(key)) addr = vmSymbolTable[key];
            else {
    addr = allocSlot(key);
    if (addr == -1) { running = false; break; }
}
            fieldTempAddrs.push_back(addr);
            savedFieldValues.push_back(memory[addr]);
        }
    }

    
    int baseAddr = -1;
    Value savedValueScalar;
    if (!isNamed) {
        if (vmSymbolTable.count(chainBase)) baseAddr = vmSymbolTable[chainBase];
        else {
    baseAddr = allocSlot(chainBase);
    if (baseAddr == -1) { running = false; break; }
}
    }

    auto runMiniExpr = [&](int startAddr) -> Value {
        int localPc = startAddr;
        while (localPc < (int)program.size()) {
            const Instruction& ei = program[localPc++];
            if (ei.op == OP_RETURN || ei.op == OP_RETURN_VAL || ei.op == OP_HALT) break;
            switch (ei.op) {
                case OP_LOAD: { Value v; v.type=VAL_INT; v.num=(long long)ei.operand; v.isInitialized=true; push(v); break; }
                case OP_LOAD_STR: { 
                    Value v; 
                    v.type = VAL_STR; 
                    v.str = (*stringPoolPtr)[ei.operand]; 
                    v.isInitialized = true; 
                    push(v); 
                    break; 
                }
                case OP_LOAD_VAR: {
                    if (ei.operand == 9999) {
                        if (vmSymbolTable.count(ei.varName)) push(memory[vmSymbolTable[ei.varName]]);
                        else { Value poison; poison.isPoisoned=true; poison.type=VAL_INT; push(poison); }
                        break;
                    }
                    int addr = ei.isLocal ? (!callStack.empty()? callStack.top().basePointer + ei.operand : ei.operand) : ei.operand;
                    push(memory[addr]); break;
                }
                case OP_LOAD_FLOAT: { 
                        Value v; 
                        v.type = VAL_FLOAT; 
                        v.decimal = (*floatPoolPtr)[ei.operand]; 
                        v.isInitialized = true; 
                        push(v); 
                        break; 
                    }
                    case OP_LOAD_BOOL: { 
                        Value v; 
                        v.type = VAL_BOOL; 
                        v.tristate = static_cast<TriState>(ei.operand); 
                        v.isInitialized = true; 
                        push(v); 
                        break; 
                    }
                
                case OP_CHAIN_FIELD_LOAD: {
                    std::string full = (*stringPoolPtr)[ei.operand];
                    if (vmSymbolTable.count(full)) push(memory[vmSymbolTable[full]]);
                    else { Value poison; poison.isPoisoned=true; poison.type=VAL_INT; push(poison); }
                    break;
                }
                case OP_ADD: { Value b=pop(),a=pop(); Value r; 
                    if (a.type==VAL_STR||b.type==VAL_STR) {
                        r.type=VAL_STR;
                        r.str = (a.type==VAL_STR?a.str:std::to_string(a.num)) + (b.type==VAL_STR?b.str:std::to_string(b.num));
                    } else { r.type=VAL_INT; r.num=a.num+b.num; }
                    r.isInitialized=true; push(r); break; }
                case OP_SUB: { Value b=pop(),a=pop(); Value r; r.type=VAL_INT; r.num=a.num-b.num; r.isInitialized=true; push(r); break; }
                case OP_MUL: { Value b=pop(),a=pop(); Value r; r.type=VAL_INT; r.num=a.num*b.num; r.isInitialized=true; push(r); break; }
                case OP_DIV: { Value b=pop(),a=pop(); Value r; r.type=VAL_INT; r.num=b.num!=0?a.num/b.num:0; r.isInitialized=true; push(r); break; }
                case OP_MOD: { Value b=pop(),a=pop(); Value r; r.type=VAL_INT; r.num=b.num!=0?a.num%b.num:0; r.isInitialized=true; push(r); break; }
                case OP_GT:  { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num>b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                case OP_LT:  { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num<b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                case OP_GTE: { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num>=b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                case OP_LTE: { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num<=b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                case OP_EQ:  { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num==b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                case OP_NEQ: { Value b=pop(),a=pop(); Value r; r.type=VAL_BOOL; r.tristate=a.num!=b.num?AST_TRUE:AST_FALSE; r.isInitialized=true; push(r); break; }
                 case OP_POWER_CALL: {
                std::string funcName = (*stringPoolPtr)[ei.operand];
                executePowerCall(funcName, ei.line);
                break;
            }
                default: break;
            }
        }
        if (!stack.empty()) return pop();
        Value def; def.type=VAL_INT; def.isInitialized=false;
        return def;
    };

    int savedPc = pc;
    std::vector<Value> results;

    for (int i = 0; i < chain.count; i++) {
        if (isNamed) {
            
            for (size_t fi = 0; fi < chain.fields.size(); fi++) {
                std::string srcKey = base + std::to_string(i+1) + ":" + chain.fields[fi];
                if (vmSymbolTable.count(srcKey)) {
                    memory[fieldTempAddrs[fi]] = memory[vmSymbolTable[srcKey]];
                    memory[fieldTempAddrs[fi]].isInitialized = true;
                }
            }
        } else {
            Value savedValueScalarTmp = memory[baseAddr];
            if (!chain.simpleAddrs.empty() && i < (int)chain.simpleAddrs.size())
                memory[baseAddr] = memory[chain.simpleAddrs[i]];
            else
                memory[baseAddr] = memory[chain.startId + i];
            memory[baseAddr].isInitialized = true;
            savedValueScalar = savedValueScalarTmp;
        }

        memory[indexAddr].type = VAL_INT;
        memory[indexAddr].num = (long long)(i + 1);
        memory[indexAddr].isInitialized = true;

        bool include = true;
        if (hasFilter && condAddr >= 0) {
            Value condResult = runMiniExpr(condAddr);
            include = (condResult.type == VAL_BOOL && condResult.tristate == AST_TRUE);
        }

        if (include) {
            Value exprResult = runMiniExpr(exprAddr);
            results.push_back(exprResult);
        }

        if (!isNamed) memory[baseAddr] = savedValueScalar;
    }

    if (isNamed) {
        for (size_t fi = 0; fi < chain.fields.size(); fi++)
            memory[fieldTempAddrs[fi]] = savedFieldValues[fi];
    }
    memory[indexAddr] = savedIndexValue;

    pc = savedPc;
    for (auto& v : results) push(v);
    Value countVal; countVal.type = VAL_INT; countVal.num = (long long)results.size(); countVal.isInitialized = true;
    push(countVal);
    break;
}
case OP_CHAIN_PRINT_FROM_STACK:{
    Value countVal = pop();
    int n = (int)countVal.num;
    std::vector<Value> vals(n);
    for (int i = n - 1; i >= 0; i--) vals[i] = pop();

    std::cout << "\033[32m";
    for (int i = 0; i < n; i++) {
        if (vals[i].type == VAL_INT) std::cout << vals[i].num;
        else if (vals[i].type == VAL_FLOAT) std::cout << formatFloat(vals[i].decimal);
        else if (vals[i].type == VAL_STR) std::cout << vals[i].str;
        if (i < n - 1) std::cout << " ";
    }
    std::cout << "\033[0m" << '\n';
    break;
}
            case OP_HALT:
    running = false;
#ifdef ASTRA_DEBUG
    if (!stack.empty()) {
        std::cerr << "[ASTRA-WARN] :: Stack not empty at halt! Size: " 
                  << stack.size() << '\n';
    }
#endif
    break;
        }

             
    if (shouldJumpToWhenEnd) {
        shouldJumpToWhenEnd = false;
        pc = whenEndAddress;
        continue;
    }
    }
}