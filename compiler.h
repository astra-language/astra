/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "common.h"
#include "lexer.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>  
#include <string>
#include <set>
#include <map>
#include <fstream>
#include <sstream>


struct Instruction {
    OpCode op;
    int operand = 0;
    int line = 0;
    bool isLocal = false;
    bool isOuter = false;
    int outerDepth = 0;
    std::string varName = ""; 
    bool flag = false;
};

struct FunctionEntry {
    int startAddress;                
    std::vector<std::string> params;
    std::vector<int> paramIds;
    int localVarCount = 0;
};

struct ModifierEntry {
    int beforeAddr = -1;
    int afterAddr  = -1;
};



struct LoopContext { int startAddr; int endAddr; int stepAddr; }; 
extern std::vector<LoopContext> loopStack;

class Compiler {
    int nextVarId = 0;
    
    std::unordered_set<std::string> constVars;

    int getVarId(const std::string& name);
    void patchJump(int index, int target, std::vector<Instruction>& bytecode);
    void generateBlock(ASTNode* node, std::vector<Instruction>& bytecode);

public:

    std::unordered_map<std::string, ModifierEntry> modifierTable;
    
    bool compilingPrint = false;
    bool compilingFunction = false;
    bool compilingLoop     = false;  
    bool compilingStore = false;

    bool hasError = false; 
    bool hasDynamicResolution = false; 
    
    std::unordered_set<std::string> localSymbols;
    std::unordered_map<std::string, int> globalSymbolTable;

    
    std::unordered_set<std::string> assignedVars; 

    std::vector<std::string> varNames;
    std::vector<std::string> stringPool;
    std::vector<double> floatPool;
    std::vector<long long> longPool;

    std::vector<Instruction> functionsBytecode;

    
    std::set<std::string> attachedFiles;
    std::map<std::string, std::string> definedFunctions; 
    std::map<std::string, std::string> aliasMap;    

    std::set<std::string> includedFiles;
    std::map<std::string, int> includeNameCount;

    std::unordered_map<std::string, FunctionEntry> functionTable;
    std::unordered_map<std::string, int> symbolTable;
    std::unordered_map<std::string, int> aliasTable;
    std::set<std::string> aliasOriginalNames; 

    std::vector<Instruction> compile(const std::vector<Token>& tokens);
    void generateCode(ASTNode* node, std::vector<Instruction>& bytecode);

    void renameVars(ASTNode* node, const std::string& alias, const std::set<std::string>& fileVars);
    std::map<std::string, std::string> chainOwner; 
    std::map<std::string, std::string> funcOwner; 

    
    void syncFromVM(const std::unordered_map<std::string, int>& vmSym,
                int vmNextId,
                const std::vector<std::string>& vmVarNames) {
    for (auto& pair : vmSym) {
        symbolTable[pair.first] = pair.second; 
        assignedVars.insert(pair.first);
    }
    if (vmVarNames.size() > varNames.size()) {
        varNames = vmVarNames;
    }
    if (vmNextId > nextVarId) nextVarId = vmNextId;
    }
};

#endif