/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "compiler.h"
#include "parser.h"
#include <iostream>
#include "error.h"
#include <algorithm>
#include <climits>

std::vector<LoopContext> loopStack;

void Compiler::generateBlock(ASTNode* node, std::vector<Instruction>& bytecode) {
    ASTNode* current = node;
    while (current != nullptr) {
        ASTNode* nextNode = current->right;
        generateCode(current, bytecode);
        current->right = nextNode;
        current = nextNode;
    }
}

int Compiler::getVarId(const std::string& name) {
    if (symbolTable.find(name) == symbolTable.end()) {
        symbolTable[name] = nextVarId++;
        varNames.push_back(name);
    }
    return symbolTable[name];
}

void Compiler::renameVars(ASTNode* node, const std::string& alias, const std::set<std::string>& fileVars) {
    if (!node) return;

    if ((node->type == NODE_VAR || node->type == NODE_STANDALONE_VAR) &&
        fileVars.count(node->varName)) {
        node->varName = alias + "." + node->varName;
    }

   
    if (node->type == NODE_OP && node->op == OP_STORE &&
        fileVars.count(node->varName)) {
        node->varName = alias + "." + node->varName;
    }
    
if ((node->type == NODE_CHAIN_DEF || 
     node->type == NODE_CHAIN_ACCESS) &&
    !node->varName.empty()) {
    
    
    std::string baseName = node->varName;
    size_t colon = baseName.find(':');
    if (colon != std::string::npos)
        baseName = baseName.substr(0, colon); 
    
    
    if (fileVars.count(baseName)) {
        
        node->varName = alias + "." + node->varName;
    }
}

    renameVars(node->left,      alias, fileVars);
    renameVars(node->right,     alias, fileVars);
    renameVars(node->condition, alias, fileVars);
    renameVars(node->body,      alias, fileVars);
    renameVars(node->startExpr, alias, fileVars);
    renameVars(node->endExpr,   alias, fileVars);
    renameVars(node->stepExpr,  alias, fileVars);
    for (auto* arg : node->arguments) renameVars(arg, alias, fileVars);
}

void Compiler::generateCode(ASTNode* node, std::vector<Instruction>& bytecode) {
    if (!node) return;

    if (node->type == NODE_CLS) {
        bytecode.push_back({OP_CLS, 0});
    }
    else if (node->type == NODE_INFO) {
        bytecode.push_back({OP_INFO, getVarId(node->varName)});
    }
    else if (node->type == NODE_INFO_CMD) {
        stringPool.push_back(node->varName);
        bytecode.push_back({OP_INFO_CMD, (int)stringPool.size() - 1});
    }
    else if (node->type == NODE_LITERAL) {
        if (node->value.type == VAL_INT) {
        if (node->value.num > INT_MAX || node->value.num < INT_MIN) {
            longPool.push_back(node->value.num);
            bytecode.push_back({OP_LOAD_LONG, (int)longPool.size() - 1, node->lineNumber});
        } else {
            bytecode.push_back({OP_LOAD, (int)node->value.num, node->lineNumber});
        }
        }
        else if (node->value.type == VAL_STR) {
            stringPool.push_back(node->value.str);
            bytecode.push_back({OP_LOAD_STR, (int)stringPool.size() - 1});
        }
        else if (node->value.type == VAL_FLOAT) {
            floatPool.push_back(node->value.decimal);
            bytecode.push_back({OP_LOAD_FLOAT, (int)floatPool.size() - 1});
        }
        else if (node->value.type == VAL_BOOL)
            bytecode.push_back({OP_LOAD_BOOL, (int)node->value.tristate});
    }
    else if (node->type == NODE_USER_INPUT) {
        stringPool.push_back(node->value.str);
        bytecode.push_back({OP_USER_INPUT, (int)stringPool.size() - 1});
    }
    else if (node->type == NODE_VAR) {
    Instruction ins;
    ins.op   = OP_LOAD_VAR;
    ins.line = node->lineNumber;
    ins.varName = node->varName; 
    if (compilingFunction) {
        if (localSymbols.count(node->varName)) {
            ins.operand = symbolTable[node->varName];
            ins.isLocal = true;
        } else if (globalSymbolTable.count(node->varName)) {
            ins.operand = globalSymbolTable[node->varName];
            ins.isLocal = false;
        } else {
            ins.operand = 9999;
            ins.isLocal = false;
        }
    } 
    
    else {
        if (assignedVars.count(node->varName) == 0) {
            ins.operand = 9999;
            ins.isLocal = false;
        }else if (symbolTable.count(node->varName) == 0) {
            ins.operand = 9999;
            ins.isLocal = false;
        }else {
            ins.operand = getVarId(node->varName);
            ins.isLocal = false;
            ins.varName = node->varName; 
        }
    }
    bytecode.push_back(ins);
}
else if (node->type == NODE_STANDALONE_VAR) {
    Instruction ins;
    ins.op      = OP_LOAD_VAR;
    ins.line    = node->lineNumber;
    ins.varName = node->varName;

    if (compilingFunction) {
        if (localSymbols.count(node->varName)) {
            ins.operand = symbolTable[node->varName];
            ins.isLocal = true;
        } else if (globalSymbolTable.count(node->varName)) {
            ins.operand = globalSymbolTable[node->varName];
            ins.isLocal = false;
        } else {
            ins.operand = 9999;
            ins.isLocal = false;
        }
    } else {
        if (assignedVars.count(node->varName) == 0) {
            ins.operand = 9999;
            ins.isLocal = false;
        } 
        else {
            ins.operand = getVarId(node->varName);
            ins.isLocal = false;
        }
    }
    bytecode.push_back(ins);

    Instruction hint;
    hint.op      = OP_HINT;
    hint.varName = node->varName;
    hint.line    = node->lineNumber;
    bytecode.push_back(hint);
}
    else if (node->type == NODE_OP) {
        if (node->op == OP_PRINT) {
    compilingPrint = true; 
    if (node->left && node->left->type == NODE_CHAIN_ACCESS) {
        generateCode(node->left, bytecode);
    }else if (node->left && node->left->type == NODE_EXE && node->left->isChainExe) {
        
        generateCode(node->left, bytecode);   
        bytecode.push_back({OP_CHAIN_PRINT_FROM_STACK, 0, node->lineNumber}); 
    } else {
        int argCount = 0;
        ASTNode* current = node->left;
        while (current != nullptr) {
            generateCode(current, bytecode);
            argCount++;
            if (current->type == NODE_OP) break;
            current = current->right;
        }
        bytecode.push_back({OP_PRINT, argCount, node->lineNumber});
    }
    compilingPrint = false; 
}
        else if (node->op == OP_STORE) {

    if (constVars.count(node->varName)) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, node->lineNumber, "Cannot reassign constant variable: " + node->varName);
        return;
    }

    compilingStore = true; 
    generateCode(node->left, bytecode);
    compilingStore = false; 

    
    if (node->isConstDef) {
        constVars.insert(node->varName);
    }

    assignedVars.insert(node->varName); 
    
    Instruction ins;
    ins.op   = OP_STORE;
    ins.line = node->lineNumber;
    
  
    ins.flag = node->isConstDef; 

    if (compilingFunction) {
    if (localSymbols.count(node->varName)) {
        if (symbolTable.find(node->varName) == symbolTable.end())
            symbolTable[node->varName] = nextVarId++;
        ins.operand = symbolTable[node->varName];
        ins.isLocal = true;
    }
    else if (globalSymbolTable.count(node->varName)) {
        
        ins.operand = globalSymbolTable[node->varName];
        ins.isLocal = false;
    }
    else {
        
        localSymbols.insert(node->varName);
        if (symbolTable.find(node->varName) == symbolTable.end())
            symbolTable[node->varName] = nextVarId++;
        ins.operand = symbolTable[node->varName];
        ins.isLocal = true;
    }
} else {
    ins.operand = getVarId(node->varName);
    ins.isLocal = false;
    ins.varName = node->varName;
}
    bytecode.push_back(ins);
}
         
        else {
            generateCode(node->left, bytecode);
            generateCode(node->right, bytecode);
            bytecode.push_back({node->op, 0});
        }
}
   
else if (node->type == NODE_WRITES) {
    for (ASTNode* arg : node->arguments) {
        generateCode(arg, bytecode);
    }
    bytecode.push_back({OP_WRITES, (int)node->arguments.size(), node->lineNumber});
}
    else if (node->type == NODE_IF) {
        compilingPrint = true; 
        generateCode(node->condition, bytecode);
        compilingPrint = false; 
        int jzIndex = bytecode.size();
        bytecode.push_back({OP_JZ, 0});
        generateBlock(node->thenBranch, bytecode);
        int jmpIndex = bytecode.size();
        bytecode.push_back({OP_JMP, 0});
        bytecode[jzIndex].operand = bytecode.size();
        if (node->elseBranch) {
            if (node->elseBranch->type == NODE_IF) generateCode(node->elseBranch, bytecode);
            else generateBlock(node->elseBranch, bytecode);
        }
        bytecode[jmpIndex].operand = bytecode.size();
    }
   
else if (node->type == NODE_REPEAT) {
    bool isLocalVar = compilingFunction;

    auto resolveVarId = [&](const std::string& name) -> int {
        if (isLocalVar) {
            localSymbols.insert(name);
            if (symbolTable.find(name) == symbolTable.end())
                symbolTable[name] = nextVarId++;
            return symbolTable[name];
        } else {
            return getVarId(name);
        }
    };

    auto makeStore = [&](int varId, int line) {
        Instruction ins;
        ins.op      = OP_STORE;
        ins.operand = varId;
        ins.line    = line;
        ins.isLocal = isLocalVar;
        return ins;
    };

    auto makeLoadVar = [&](int varId, int line) {
        Instruction ins;
        ins.op      = OP_LOAD_VAR;
        ins.operand = varId;
        ins.line    = line;
        ins.isLocal = isLocalVar;
        return ins;
    };

    
    generateCode(node->startExpr, bytecode);
    int varId = resolveVarId(node->iteratorName);
    assignedVars.insert(node->iteratorName);
    bytecode.push_back(makeStore(varId, node->lineNumber));

    
    generateCode(node->endExpr, bytecode);
    std::string limitName = "__limit_" + std::to_string(node->lineNumber);
    int limitId = resolveVarId(limitName);
    bytecode.push_back(makeStore(limitId, node->lineNumber));

    
    generateCode(node->stepExpr, bytecode);
    std::string stepName = "__step_" + std::to_string(node->lineNumber);
    int stepId = resolveVarId(stepName);
    bytecode.push_back(makeStore(stepId, node->lineNumber));

    int loopStart = bytecode.size();

    bytecode.push_back(makeLoadVar(varId,   node->lineNumber));
    bytecode.push_back(makeLoadVar(limitId, node->lineNumber));
    bytecode.push_back(makeLoadVar(stepId,  node->lineNumber));

    int loopCondIndex = bytecode.size();
    bytecode.push_back({OP_LOOP_COND, 0, node->lineNumber});

   
    loopStack.push_back({loopStart, 0, 0});
    compilingLoop = true;
    generateBlock(node->body, bytecode);
    compilingLoop = false;

    int loopContinue = bytecode.size();
    loopStack.back().stepAddr = loopContinue;

    //bytecode.push_back(makeLoadVar(varId,  node->lineNumber));
    bytecode.push_back(makeLoadVar(stepId, node->lineNumber));

    {
        Instruction stepInstr;
        stepInstr.op      = OP_LOOP_STEP;
        stepInstr.operand = varId;
        stepInstr.line    = node->lineNumber;
        stepInstr.isLocal = isLocalVar;     
        bytecode.push_back(stepInstr);
    }

    bytecode.push_back({OP_JMP, loopStart, node->lineNumber});

    int loopEnd = bytecode.size();
    bytecode[loopCondIndex].operand = loopEnd;
    loopStack.back().endAddr = loopEnd;

    for (auto& instr : bytecode) {
        if (instr.op == OP_BREAK && instr.operand == -1) instr.operand = loopEnd;
    }
    for (auto& instr : bytecode) {
        if (instr.op == OP_CONTINUE && instr.operand == -1) instr.operand = loopContinue;
    }

    loopStack.pop_back();
}
   else if (node->type == NODE_REPEAT_COND) {
    int loopStart = bytecode.size();
    compilingPrint = true; 
    generateCode(node->condition, bytecode);
    compilingPrint = false; 
    int jzIndex = bytecode.size();
    bytecode.push_back({OP_JZ, 0});
    compilingLoop = true;   
    loopStack.push_back({loopStart, 0, 0});
    loopStack.back().stepAddr = loopStart;

    generateBlock(node->body, bytecode);

    bytecode.push_back({OP_JMP, loopStart});

    int loopEnd = bytecode.size();
    bytecode[jzIndex].operand = loopEnd;
    loopStack.back().endAddr = loopEnd;

    for (auto& instr : bytecode) {
        if (instr.op == OP_BREAK    && instr.operand == -1) instr.operand = loopEnd;
    }
    for (auto& instr : bytecode) {
        if (instr.op == OP_CONTINUE && instr.operand == -1) instr.operand = loopStart;
    }

    loopStack.pop_back();
    compilingLoop = false;  
}
    else if (node->type == NODE_POWER_CALL) {
        for (ASTNode* arg : node->arguments) generateCode(arg, bytecode);
        stringPool.push_back(node->varName);
        bytecode.push_back({OP_POWER_CALL, (int)stringPool.size() - 1, node->lineNumber});
    }
    else if (node->type == NODE_NEGATE) {
        generateCode(node->left, bytecode);
        bytecode.push_back({OP_NEG, 0});
    }
    else if (node->type == NODE_NOT) {
        generateCode(node->left, bytecode);
        bytecode.push_back({OP_NOT, 0});
    }
    else if (node->type == NODE_FUNC_DEF) {
    std::vector<Instruction> funcCode;
    funcCode.push_back({OP_JMP, 0}); 

    FunctionEntry entry;
    entry.params = node->params;

    std::unordered_map<std::string, int> savedSymbolTable = symbolTable;
    int savedNextVarId = nextVarId;
    bool savedCompilingFunction = compilingFunction;
    std::unordered_set<std::string> savedLocalSymbols = localSymbols;

    for (auto& pair : symbolTable) globalSymbolTable[pair.first] = pair.second;
    for (auto& name : localSymbols)
        if (symbolTable.count(name))
            globalSymbolTable[name] = symbolTable[name];

    symbolTable.clear();
    nextVarId = 0;
    localSymbols.clear();
    compilingFunction = true;

   
    for (auto& [aliasName, globalAddr] : aliasTable) {
        symbolTable[aliasName] = nextVarId++;
        localSymbols.insert(aliasName);
        Instruction loadGlobal;
        loadGlobal.op = OP_LOAD_VAR;
        loadGlobal.operand = globalAddr;
        loadGlobal.isLocal = false;
        funcCode.push_back(loadGlobal);

        Instruction storeLocal;
        storeLocal.op = OP_STORE;
        storeLocal.operand = symbolTable[aliasName];
        storeLocal.isLocal = true;
        funcCode.push_back(storeLocal);
    }
    int aliasCount = (int)aliasTable.size();
    for (int i = 0; i < (int)entry.params.size(); i++) {
        symbolTable[entry.params[i]] = aliasCount + i;  
        localSymbols.insert(entry.params[i]);
    }
    nextVarId = aliasCount + (int)entry.params.size();    
    for (auto& p : entry.params) entry.paramIds.push_back(symbolTable[p]);

    generateBlock(node->body, funcCode);
    entry.localVarCount = nextVarId;

    compilingFunction = savedCompilingFunction;
    symbolTable = savedSymbolTable;
    nextVarId = savedNextVarId;
    localSymbols = savedLocalSymbols;

    funcCode.push_back({OP_RETURN, 0});

    int funcStartOffset = functionsBytecode.size();
    entry.startAddress = funcStartOffset + 1; 

    
    for (int i = 1; i < (int)funcCode.size(); i++) {
        auto& instr = funcCode[i];
        if (instr.op == OP_JMP || instr.op == OP_JZ || instr.op == OP_LOOP_COND) {
            instr.operand += funcStartOffset;
        }
    }

    
    funcCode[0].operand = funcStartOffset + (int)funcCode.size();

    functionTable[node->varName + "_" + std::to_string(entry.params.size())] = entry;
    functionsBytecode.insert(functionsBytecode.end(), funcCode.begin(), funcCode.end());
}
    else if (node->type == NODE_FUNC_CALL) {
    if (node->varName == "adr") {
    if (node->arguments.size() == 2) {
        if (node->arguments[0]->type != NODE_VAR) {
        std::cerr << RED << "[ASTRA-SYN-ERR]" << RESET
                  << " :: adr() first argument must be a variable name." << std::endl;
    }
        
        generateCode(node->arguments[1], bytecode); 

        std::string targetVar = node->arguments[0]->varName;
        Instruction ins;
        ins.op = OP_CALL_ADR;
        ins.line = node->lineNumber;
        ins.flag = true; 

        if (compilingFunction) {
            if (localSymbols.count(targetVar)) {
                ins.operand = symbolTable[targetVar];
                ins.isLocal = true;
            } else if (globalSymbolTable.count(targetVar)) {
                ins.operand = globalSymbolTable[targetVar];
                ins.isLocal = false;
            } else {
                ins.operand = getVarId(targetVar);
                ins.isLocal = false;
            }
        } else {
            ins.operand = getVarId(targetVar);
            ins.isLocal = false;
        }

        bytecode.push_back(ins);
    }
    else {
        std::string targetVar = node->arguments[0]->varName;
        Instruction ins;
        ins.op = OP_CALL_ADR;
        ins.line = node->lineNumber;
        ins.flag = false; 

        if (compilingFunction) {
            if (localSymbols.count(targetVar)) {
                ins.operand = symbolTable[targetVar];
                ins.isLocal = true;
            } else if (globalSymbolTable.count(targetVar)) {
                ins.operand = globalSymbolTable[targetVar];
                ins.isLocal = false;
            } else {
                ins.operand = getVarId(targetVar);
                ins.isLocal = false;
            }
        } else {
            ins.operand = getVarId(targetVar);
            ins.isLocal = false;
        }

        bytecode.push_back(ins);
    }
}
    else if (node->varName == "val") {
       
        generateCode(node->arguments[0], bytecode);
        bytecode.push_back({OP_CALL_VAL, 0});
    }
    else if (node->varName == "create") {
    generateCode(node->arguments[0], bytecode); 
    bytecode.push_back({OP_FILE_CREATE, 0, node->lineNumber});
}
else if (node->varName == "close") {
    generateCode(node->arguments[0], bytecode); 
    bytecode.push_back({OP_FILE_CLOSE, 0, node->lineNumber});
}
else if (node->varName == "plus") {
    
    generateCode(node->arguments[0], bytecode); 
    generateCode(node->arguments[1], bytecode); 
    generateCode(node->arguments[2], bytecode); 
    bytecode.push_back({OP_FILE_PLUS, 0, node->lineNumber});
}
else if (node->varName == "read") {
    
    generateCode(node->arguments[0], bytecode); 
    generateCode(node->arguments[1], bytecode); 
    bytecode.push_back({OP_FILE_READ, 0, node->lineNumber});
}
else if (node->varName == "eof") {
    generateCode(node->arguments[0], bytecode);
    bytecode.push_back({OP_FILE_EOF, 0, node->lineNumber});
}
else if (node->varName == "fetch") {
    generateCode(node->arguments[0], bytecode);
    if (node->arguments.size() == 2) {
        bytecode.push_back({OP_LOAD, 0});
        bytecode.push_back({OP_LOAD, 0});
        generateCode(node->arguments[1], bytecode);
    }
    else if (node->arguments.size() == 3) {
        generateCode(node->arguments[1], bytecode);
        bytecode.push_back({OP_LOAD, 0});
        generateCode(node->arguments[2], bytecode);
    }
    else if (node->arguments.size() == 4) {
        generateCode(node->arguments[1], bytecode);
        generateCode(node->arguments[2], bytecode);
        generateCode(node->arguments[3], bytecode);
    }
    bytecode.push_back({OP_FILE_FETCH, 0, node->lineNumber});
}
else if (node->varName == "parseJson") {
    stringPool.push_back(node->arguments[0]->value.str);
    bytecode.push_back({OP_LOAD_STR, (int)stringPool.size() - 1});
    generateCode(node->arguments[1], bytecode);
    bytecode.push_back({OP_JSON_PARSE, 0, node->lineNumber});
}
else if (node->varName == "toJson") {
    stringPool.push_back(node->arguments[0]->varName); 
    bytecode.push_back({OP_TO_JSON, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "jsonpretty") {
    generateCode(node->arguments[0], bytecode);
    bytecode.push_back({OP_JSON_PRETTY, 0, node->lineNumber});
}
else if (node->varName == "clear") {
    generateCode(node->arguments[0], bytecode); 

    if (node->arguments.size() == 1) {
        
        bytecode.push_back({OP_FILE_CLEAR, 0, node->lineNumber});
    }
    else if (node->arguments.size() == 2) {
        
        generateCode(node->arguments[1], bytecode); 
        bytecode.push_back({OP_FILE_CLEAR, 1, node->lineNumber}); 
    }
}
    else {
    
    std::string baseName = node->varName;
    size_t underscore = baseName.rfind('_');
    if (underscore != std::string::npos) {
        std::string suffix = baseName.substr(underscore + 1);
        bool allDigits = !suffix.empty() &&
                         std::all_of(suffix.begin(), suffix.end(), ::isdigit);
        if (allDigits) baseName = baseName.substr(0, underscore); 
    }

    if (funcOwner.count(baseName)) {
        if (funcOwner[baseName] == "__conflict__") {
            AstraError::syntax(ErrCode::INVALID_SYNTAX, node->lineNumber,
                "Ambiguous function '" + baseName + "'. Use alias to specify.");
            hasError = true;
            return;
        }
        
        std::string owner = funcOwner[baseName];
    if (!owner.empty()) { 
        node->varName = owner + "." + node->varName;
    }
    }

    for (ASTNode* arg : node->arguments) generateCode(arg, bytecode);
    stringPool.push_back(node->varName + "_" + std::to_string(node->arguments.size()));
    bytecode.push_back({OP_FUNC_CALL, (int)stringPool.size() - 1, node->lineNumber});
}
}
    else if (node->type == NODE_RETURN) {
        generateCode(node->left, bytecode);
        bytecode.push_back({OP_RETURN_VAL, 0});
    }
    else if (node->type == NODE_CHAIN_DEF) {
    bool isSinglePowerCall = (node->arguments.size() == 1 && 
                               (node->arguments[0]->type == NODE_POWER_CALL ||
                                (node->arguments[0]->type == NODE_EXE && node->arguments[0]->isChainExe)));
    
    for (ASTNode* arg : node->arguments) generateCode(arg, bytecode);
    
    
    if (!isSinglePowerCall) {
        bytecode.push_back({OP_LOAD, (int)node->arguments.size()});
    }

    std::string base = node->varName.substr(0, node->varName.find(':'));
    int count = (int)node->arguments.size();
    for (int i = 1; i <= count; i++) {
        assignedVars.insert(base + std::to_string(i));
    }
    
    std::string fieldStr = "";
    for (int i = 0; i < (int)node->params.size(); i++) {
        if (i > 0) fieldStr += ",";
        fieldStr += node->params[i];
    }
    stringPool.push_back(node->varName + "|" + fieldStr);
    bytecode.push_back({OP_CHAIN_STORE, (int)stringPool.size() - 1});
}
    else if (node->type == NODE_CHAIN_ACCESS) {
  
    std::string base = node->varName;
    size_t colon = base.find(':');
    if (colon != std::string::npos) base = base.substr(0, colon);

    if (chainOwner.count(base)) {
        if (chainOwner[base] == "__conflict__") {
            AstraError::syntax(ErrCode::INVALID_SYNTAX, node->lineNumber,
                "Ambiguous chain '" + node->varName + "'. Use alias to specify.");
            hasError = true;
            return;
        }
        
        std::string owner = chainOwner[base];
        node->varName = owner + "." + node->varName;
    }

    stringPool.push_back(node->varName);
    bytecode.push_back({OP_CHAIN_PRINT, (int)stringPool.size() - 1});
}
    else if (node->type == NODE_CHAIN_FIELD_ACCESS) {
        stringPool.push_back(node->varName);
        bytecode.push_back({OP_CHAIN_FIELD_LOAD, (int)stringPool.size() - 1});
    }
    else if (node->type == NODE_CHAIN_FIELD_STORE) {
        generateCode(node->left, bytecode);
        stringPool.push_back(node->varName);
        bytecode.push_back({OP_CHAIN_FIELD_STORE, (int)stringPool.size() - 1});
    }
    else if (node->type == NODE_CHAIN_DYNAMIC) {
        
        generateCode(node->left, bytecode);
        stringPool.push_back(node->varName);
        bytecode.push_back({OP_CHAIN_DYNAMIC_LOAD, (int)stringPool.size() - 1, node->lineNumber});
    }
  else if (node->type == NODE_CHAIN_DYNAMIC_STORE) {
    
    generateCode(node->condition, bytecode);  
    
    
    generateCode(node->left, bytecode); 

    stringPool.push_back(node->varName);
    bytecode.push_back({OP_CHAIN_DYNAMIC_STORE, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->type == NODE_CHAIN_FUNC) {
    if (node->varName == "len") {
        stringPool.push_back(node->params[0]);
        bytecode.push_back({OP_CHAIN_LEN, (int)stringPool.size() - 1, node->lineNumber});
    }
    else if (node->varName == "sort") {
        stringPool.push_back(node->params[0]);
        bytecode.push_back({OP_CHAIN_SORT, (int)stringPool.size() - 1, node->lineNumber});
    }
    else if (node->varName == "merge") {
        stringPool.push_back(node->params[0] + "|" + node->params[1]);
        bytecode.push_back({OP_CHAIN_MERGE, (int)stringPool.size() - 1, node->lineNumber});
    }
    else if (node->varName == "unique") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_UNIQUE, (int)stringPool.size() - 1, node->lineNumber});
   }
   else if (node->varName == "self") {
    for (auto& arg : node->arguments) generateCode(arg, bytecode);
    bytecode.push_back({OP_LOAD, (int)node->arguments.size()});
    stringPool.push_back("self|" + node->params[0]);
    bytecode.push_back({OP_CHAIN_SELF, (int)stringPool.size() - 1});
}else if (node->varName == "sum") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_SUM, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "avg") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_AVG, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "chainMax") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_MAX, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "chainMin") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_MIN, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "reverse") {
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_REVERSE, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "contains") {
    generateCode(node->arguments[0], bytecode);   
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_CONTAINS, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "indexOf") {
    generateCode(node->arguments[0], bytecode);
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_INDEXOF, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->varName == "join") {
    generateCode(node->arguments[0], bytecode);   
    stringPool.push_back(node->params[0]);
    bytecode.push_back({OP_CHAIN_JOIN, (int)stringPool.size() - 1, node->lineNumber});
}
}
else if (node->type == NODE_CHAIN_DYNAMIC_FIELD_STORE) {
    generateCode(node->condition, bytecode);  
    generateCode(node->left, bytecode);        
    std::string key = node->varName + "|" + node->params[0]; 
    stringPool.push_back(key);
    bytecode.push_back({OP_CHAIN_DYNAMIC_FIELD_STORE, (int)stringPool.size() - 1});
}
else if (node->type == NODE_CHAIN_DYNAMIC_FIELD_LOAD) {
    generateCode(node->left, bytecode); 
    std::string key = node->varName + "|" + node->params[0]; 
    stringPool.push_back(key);
    bytecode.push_back({OP_CHAIN_DYNAMIC_FIELD_LOAD, (int)stringPool.size() - 1});
}
else if (node->type == NODE_CHAIN_INFO) {
    stringPool.push_back(node->varName);
    bytecode.push_back({OP_CHAIN_INFO, (int)stringPool.size() - 1});
}
   else if (node->type == NODE_BREAK) {
        bytecode.push_back({OP_BREAK, -1, node->lineNumber}); 
    }
    else if (node->type == NODE_CONTINUE) {
    if (!loopStack.empty()) {
    
        bytecode.push_back({OP_CONTINUE, -1, node->lineNumber});
    }
}
else if (node->type == NODE_METHOD_DEF) {
    std::vector<Instruction> funcCode;
    funcCode.push_back({OP_JMP, 0});

    FunctionEntry entry;
    entry.params = node->params;

    std::unordered_map<std::string, int> savedSymbolTable = symbolTable;
    int savedNextVarId = nextVarId;
    bool savedCompilingFunction = compilingFunction;
    std::unordered_set<std::string> savedLocalSymbols = localSymbols;

    for (auto& pair : symbolTable) globalSymbolTable[pair.first] = pair.second;

    symbolTable.clear();
    nextVarId = 0;
    localSymbols.clear();
    compilingFunction = true;

    for (int i = 0; i < (int)entry.params.size(); i++) {
        symbolTable[entry.params[i]] = i;
        localSymbols.insert(entry.params[i]);
    }
    nextVarId = (int)entry.params.size();
    for (auto& p : entry.params) entry.paramIds.push_back(symbolTable[p]);

    generateBlock(node->body, funcCode);

    compilingFunction = savedCompilingFunction;
    symbolTable = savedSymbolTable;
    nextVarId = savedNextVarId;
    localSymbols = savedLocalSymbols;

    funcCode.push_back({OP_RETURN, 0});

    int funcStartOffset = functionsBytecode.size();
    entry.startAddress = funcStartOffset + 1;

    for (int i = 1; i < (int)funcCode.size(); i++) {
        auto& instr = funcCode[i];
        if (instr.op == OP_JMP || instr.op == OP_JZ || instr.op == OP_LOOP_COND)
            instr.operand += funcStartOffset;
    }
    funcCode[0].operand = funcStartOffset + (int)funcCode.size();

    
    functionTable[node->varName] = entry;
    functionsBytecode.insert(functionsBytecode.end(), funcCode.begin(), funcCode.end());
}
else if (node->type == NODE_METHOD_CALL) {
    
    for (auto& arg : node->arguments) generateCode(arg, bytecode);
    stringPool.push_back(node->varName); 
    bytecode.push_back({OP_METHOD_CALL, (int)stringPool.size() - 1});
}
else if (node->type == NODE_CHAIN_DYNAMIC_METHOD_CALL) {
    generateCode(node->left, bytecode); 
    for (auto& arg : node->arguments) generateCode(arg, bytecode);
    std::string key = node->varName + "|" + node->params[0];
    stringPool.push_back(key);
    bytecode.push_back({OP_CHAIN_DYNAMIC_METHOD_CALL, (int)stringPool.size() - 1});
}
else if (node->type == NODE_CHECK) {
    
    generateCode(node->left, bytecode);
    
    std::vector<int> jumpToEnds;
    
    for (auto* caseNode : node->arguments) {
        if (caseNode->value.type == VAL_INT && caseNode->value.str != "__range__" && caseNode->value.str != "__default__") {
            
            bytecode.push_back({OP_CHECK_INT, (int)caseNode->value.num});
            int jzIdx = bytecode.size();
            bytecode.push_back({OP_JZ, 0});
            generateBlock(caseNode->left, bytecode);
            jumpToEnds.push_back(bytecode.size());
            bytecode.push_back({OP_JMP, 0});
            bytecode[jzIdx].operand = bytecode.size();
        }
        else if (caseNode->value.type == VAL_STR && caseNode->value.str == "__range__") {
            
            int start = (int)caseNode->value.num;
            int end   = (int)caseNode->right->value.num;
            bytecode.push_back({OP_CHECK_RANGE, start});
            Instruction ins; ins.op = OP_CHECK_RANGE; ins.operand = start; ins.line = end;
            bytecode.back() = ins;
            int jzIdx = bytecode.size();
            bytecode.push_back({OP_JZ, 0});
            generateBlock(caseNode->left, bytecode);
            jumpToEnds.push_back(bytecode.size());
            bytecode.push_back({OP_JMP, 0});
            bytecode[jzIdx].operand = bytecode.size();
        }
        else if (caseNode->value.type == VAL_STR && caseNode->value.str == "__default__") {
            
            generateBlock(caseNode->left, bytecode);
        }
        else if (caseNode->value.type == VAL_STR) {
            
            stringPool.push_back(caseNode->value.str);
            bytecode.push_back({OP_CHECK_STR, (int)stringPool.size() - 1});
            int jzIdx = bytecode.size();
            bytecode.push_back({OP_JZ, 0});
            generateBlock(caseNode->left, bytecode);
            jumpToEnds.push_back(bytecode.size());
            bytecode.push_back({OP_JMP, 0});
            bytecode[jzIdx].operand = bytecode.size();
        }
    }
    
    
    int endIdx = bytecode.size();
    for (int idx : jumpToEnds) bytecode[idx].operand = endIdx;
    
    
    bytecode.push_back({OP_CHECK_END, 0});
}
else if (node->type == NODE_WHEN) {
    
    bytecode.push_back({OP_WHEN_START, 0, node->lineNumber});
    int whenStartIdx = bytecode.size() - 1;

   
    generateBlock(node->body, bytecode);

    
    int whenEndIdx = bytecode.size();
    bytecode.push_back({OP_WHEN_END, 0, node->lineNumber});

    
    bytecode[whenStartIdx].operand = whenEndIdx; 
    std::vector<int> thenEndJumps;

    
    for (auto* thenNode : node->arguments) {
        
        ErrCode code = ErrCode::NONE; 
        if (!thenNode->varName.empty()) {
            code = AstraError::fromString(thenNode->varName);
        }

        int thenCheckIdx = bytecode.size();
        Instruction thenCheck;
        thenCheck.op      = OP_THEN_CHECK;
        thenCheck.operand = (int)code;
        thenCheck.line    = 0; 
        bytecode.push_back(thenCheck);

       
        generateBlock(thenNode->body, bytecode);

        
        thenEndJumps.push_back(bytecode.size());
        bytecode.push_back({OP_THEN_END, 0, node->lineNumber});

        
        bytecode[thenCheckIdx].line = bytecode.size();
    }

    
    int endAddr = bytecode.size();
    bytecode[whenEndIdx].operand = endAddr;
    for (int idx : thenEndJumps) bytecode[idx].operand = endAddr;
}
else if (node->type == NODE_ATTACH) {
    std::string filePath = node->varName;
    std::string alias    = node->params.empty() ? "" : node->params[0];

   
    if (attachedFiles.count(filePath)) {
        std::cout << MAGENTA << "[ASTRA-INFO]" << RESET
                  << " :: '" << filePath << "' already attached." << std::endl;
        return;
    }

    
    if (!alias.empty() && assignedVars.count(alias)) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, node->lineNumber,
                          "'" + alias + "' already used as a variable");
        hasError = true;
        return;
    }

    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, node->lineNumber,
                           "Cannot open '" + filePath + "'");
        hasError = true;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    attachedFiles.insert(filePath);

    Lexer lexer(buffer.str());
    auto attachTokens = lexer.tokenize();
    Parser attachParser(attachTokens);

    
    std::vector<ASTNode*> allNodes;
    std::set<std::string> fileVars;

    while (attachParser.getCurrentPos() < attachTokens.size()) {
    Token t = attachParser.peek();
    if (t.type == TOKEN_EOF) break;
    ASTNode* nd = attachParser.parseStatement();
    if (!nd) continue;
    allNodes.push_back(nd);

    
    if (nd->type == NODE_OP && nd->op == OP_STORE) {
        fileVars.insert(nd->varName);
    }
    
    if (nd->type == NODE_CHAIN_DEF) {
    fileVars.insert(nd->varName);
    
    std::string chainKey = nd->varName; 
    if (chainOwner.count(chainKey)) {
        
        chainOwner[chainKey] = "__conflict__";
    } else {
        chainOwner[chainKey] = alias; 
    }
  }
}

    
    if (!alias.empty()) {
        for (auto* nd : allNodes) {
            renameVars(nd, alias, fileVars);
        }
    }

    
for (auto* nd : allNodes) {
    if (nd->type == NODE_FUNC_DEF) {
       
    std::string baseName = nd->varName;
    if (funcOwner.count(baseName)) {
        funcOwner[baseName] = "__conflict__";
    } else {
        funcOwner[baseName] = alias;
    }
        if (alias.empty()) {
            if (definedFunctions.count(nd->varName) &&
                definedFunctions[nd->varName] != filePath) {
                AstraError::syntax(ErrCode::INVALID_SYNTAX, node->lineNumber,
                                  "'" + nd->varName + "' already defined in '" +
                                  definedFunctions[nd->varName] +
                                  "'. Use: attach \"" + filePath + "\" as <alias>");
                hasError = true;
                return;
            }
            definedFunctions[nd->varName] = filePath;
            generateCode(nd, bytecode);
        } else {
            nd->varName = alias + "." + nd->varName;
            generateCode(nd, bytecode);
            aliasMap[alias] = filePath;
        }
    }
    else if (nd->type == NODE_CHAIN_DEF) {
        
        generateCode(nd, bytecode);
    }
    else if (nd->type == NODE_OP && nd->op == OP_STORE) {
        if (nd->left && nd->left->type == NODE_USER_INPUT) {
            
        } else {
            generateCode(nd, bytecode);
        }
    }
    else if (nd->type == NODE_OP && nd->op == OP_PRINT) {
        
    }
    else if (nd->type == NODE_WRITES) {
        
    }
    else if (nd->type == NODE_IF   ||
             nd->type == NODE_REPEAT ||
             nd->type == NODE_REPEAT_COND) {
        
    }
    else {
        generateCode(nd, bytecode);
    }
  }
 }
else if (node->type == NODE_INCLUDE) {
    std::string filePath = node->varName;
    std::string mode = node->value.str;
    std::set<std::string> nameList(node->params.begin(), node->params.end());

    std::string dedupKey = filePath + "::" + mode;
    for (const auto& n : nameList) dedupKey += "," + n;

    if (includedFiles.count(dedupKey)) {
        std::cout << MAGENTA << "[ASTRA-INFO]" << RESET
                  << " :: '" << filePath << "' (this selector) already included." << std::endl;
        return;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, node->lineNumber,
                           "Cannot open '" + filePath + "'");
        hasError = true;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    includedFiles.insert(dedupKey);

    Lexer includeLexer(buffer.str());
    auto includeTokens = includeLexer.tokenize();
    Parser includeParser(includeTokens);
    std::vector<ASTNode*> allNodes;
    while (includeParser.getCurrentPos() < includeTokens.size()) {
        Token tk = includeParser.peek();
        if (tk.type == TOKEN_EOF) break;
        ASTNode* nd = includeParser.parseStatement();
        if (!nd) continue;
        allNodes.push_back(nd);
    }

    for (auto* nd : allNodes) {
        std::string ndName = "";
        bool isNamed = false;
        if (nd->type == NODE_FUNC_DEF) { ndName = nd->varName; isNamed = true; }
        else if (nd->type == NODE_CHAIN_DEF) { ndName = nd->varName; isNamed = true; }
        else if (nd->type == NODE_OP && nd->op == OP_STORE) { ndName = nd->varName; isNamed = true; }

        if (mode == "only") {
            if (!isNamed || !nameList.count(ndName)) continue;
        } else if (mode == "except") {
            if (isNamed && nameList.count(ndName)) continue;
        }

        if (nd->type == NODE_FUNC_DEF) {
            std::string baseName = nd->varName;
            int& cnt = includeNameCount[baseName];
            cnt++;
            if (cnt > 1) {
                std::string newName = baseName + "_" + std::to_string(cnt);
                std::cout << MAGENTA << "[ASTRA-INFO]" << RESET
                          << " :: '" << baseName << "' already included from another file. Renamed to '"
                          << newName << "'." << std::endl;
                nd->varName = newName;
            }
        }
        generateCode(nd, bytecode);
    }
}
else if (node->type == NODE_DEALIAS) {
    
    stringPool.push_back(node->varName);
    bytecode.push_back({OP_DEALIAS, (int)stringPool.size() - 1, node->lineNumber});
}
else if (node->type == NODE_MODIFIER_DEF) {
    
    int jmpIndex = functionsBytecode.size();
    functionsBytecode.push_back({OP_JMP, 0});  
    
    std::vector<Instruction> beforeCode;
    if (node->body) {
        generateBlock(node->body, beforeCode);
    }
    
    std::vector<Instruction> afterCode;
    if (node->condition) {
        generateBlock(node->condition, afterCode);
    }
    
    ModifierEntry entry;
    
    entry.beforeAddr = functionsBytecode.size();
    functionsBytecode.insert(functionsBytecode.end(), beforeCode.begin(), beforeCode.end());
    functionsBytecode.push_back({OP_RETURN, 0});
    
    entry.afterAddr = functionsBytecode.size();
    functionsBytecode.insert(functionsBytecode.end(), afterCode.begin(), afterCode.end());
    functionsBytecode.push_back({OP_RETURN, 0});
    
    functionsBytecode[jmpIndex].operand = functionsBytecode.size();
    
    modifierTable[node->varName] = entry;
}
else if (node->type == NODE_MODIFIER_CALL) {
    for (auto& modName : node->params) {
        if (modifierTable.count(modName)) {
            stringPool.push_back(modName + "_before");
            bytecode.push_back({OP_MODIFIER_CALL, (int)stringPool.size() - 1});
        }
    }
    
    generateCode(node->left, bytecode);
    
    for (int i = (int)node->params.size() - 1; i >= 0; i--) {
        if (modifierTable.count(node->params[i])) {
            stringPool.push_back(node->params[i] + "_after");
            bytecode.push_back({OP_MODIFIER_CALL, (int)stringPool.size() - 1});
        }
    }
}
else if (node->type == NODE_EXE) {
    if (node->isChainExe) {
        std::string chainBase = node->varName.substr(0, node->varName.find(':'));
        assignedVars.insert(chainBase);
        if (symbolTable.find(chainBase) == symbolTable.end()) {
            symbolTable[chainBase] = nextVarId++;
            varNames.push_back(chainBase);
        }

        int jmpIdx = bytecode.size();
        bytecode.push_back({OP_JMP, 0});

       
        int exprStart = bytecode.size();
        generateCode(node->left, bytecode);
        bytecode.push_back({OP_RETURN, 0});

        
        int condStart = -1;
        if (node->condition) {
            condStart = bytecode.size();
            generateCode(node->condition, bytecode);
            bytecode.push_back({OP_RETURN, 0});
        }

      
        bytecode[jmpIdx].operand = bytecode.size();

        
        stringPool.push_back(node->varName);
        stringPool.push_back(chainBase);

        Instruction ins;
        ins.op = OP_EXE_CHAIN;
        ins.operand = (int)stringPool.size() - 2;
        ins.line = exprStart;
        ins.flag = (node->condition != nullptr);   
        ins.outerDepth = condStart;                  
        bytecode.push_back(ins);
    }
    else {
        generateCode(node->left, bytecode);
    }
}
else if (node->type == NODE_LINK) {
    stringPool.push_back(node->varName); 
    int sourceIdx = (int)stringPool.size() - 1;

    std::string targetsStr = "";
    for (size_t i = 0; i < node->params.size(); i++) {
        if (i > 0) targetsStr += ",";
        targetsStr += node->params[i];
        assignedVars.insert(node->params[i]);
        getVarId(node->params[i]); 
    }
    stringPool.push_back(targetsStr); 
    
    assignedVars.insert(node->varName);
    getVarId(node->varName); 

    Instruction ins;
    ins.op = OP_LINK;
    ins.operand = sourceIdx;
    ins.line = node->lineNumber;
    bytecode.push_back(ins);
    hasDynamicResolution = true;
}
}

std::vector<Instruction> Compiler::compile(const std::vector<Token>& tokens) {
    std::vector<Instruction> bytecode;
    Parser parser(tokens);

    while (parser.getCurrentPos() < tokens.size()) {
        Token t = parser.peek();
        if (t.type == TOKEN_EOF) break;
         if (t.type == TOKEN_SEMICOLON) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, t.line,
            " Unexpected ';' not after individual statements");
        hasError = true;
        parser.consume();   
        continue;
    }

        if (t.type == TOKEN_FUNC_START) {
            generateCode(parser.parseFuncDef(), bytecode);
        }
        else if (t.type == TOKEN_IF) {
            generateCode(parser.parseIfStatement(), bytecode);
        }
        else if (t.type == TOKEN_VAR &&
                 parser.peekAt(parser.getCurrentPos() + 1).type == TOKEN_ASSIGN) {
            std::string varName = t.value;
            parser.consume();
            parser.consume();
            ASTNode* exprNode = parser.parseComparison();
    
    
    if (parser.peek().type == TOKEN_LBRACKET) {
        parser.consume();
        ASTNode* modNode = new ASTNode();
        modNode->type = NODE_MODIFIER_CALL;
        modNode->left = exprNode;
        
        while (parser.peek().type != TOKEN_RBRACKET && 
               parser.peek().type != TOKEN_EOF) {
            Token modName = parser.consume(TOKEN_VAR, "Expected modifier name");
            modNode->params.push_back(modName.value);
            if (parser.peek().type == TOKEN_COMMA) parser.consume();
        }
        parser.consume(); 
        generateCode(modNode, bytecode);
    } else {
        generateCode(exprNode, bytecode);
    }
            Instruction ins;
            ins.op      = OP_STORE;
            ins.operand = getVarId(varName);
            ins.line    = 0;
            ins.isLocal = compilingFunction;
            ins.varName = varName;
            assignedVars.insert(varName); 
            bytecode.push_back(ins);
            
if (parser.peek().type == TOKEN_ALIAS) {
    parser.consume();
    Token aliasTok = parser.consume(TOKEN_VAR);
    aliasTable[aliasTok.value] = ins.operand;
    assignedVars.insert(aliasTok.value);
    aliasOriginalNames.insert(varName);
    stringPool.push_back(aliasTok.value); 
    stringPool.push_back(varName);        
    bytecode.push_back({OP_ALIAS, (int)stringPool.size() - 2, 0});
    hasDynamicResolution = true;    
}
}
else if (t.type == TOKEN_ATTACH) {
    ASTNode* nd = parser.parseStatement();
    generateCode(nd, bytecode);
    hasDynamicResolution = true;   
}else if (t.type == TOKEN_DEALIAS) {
    parser.consume();
    Token nameTok = parser.consume(TOKEN_VAR);
    aliasTable.erase(nameTok.value);
    assignedVars.insert(nameTok.value); 
    hasDynamicResolution = true;   
}
else {
    ASTNode* nd = parser.parseStatement();
    
    ASTNode* curr = nd;
    while (curr != nullptr) {
        ASTNode* nextNd = curr->right;
        generateCode(curr, bytecode);
        curr = nextNd;
    }
    }
}
    
    
    if (parser.hasError || hasError) {
        return {};
    }

    int offset = functionsBytecode.size();
if (offset > 0) {
    for (auto& instr : bytecode) {
        if (instr.op == OP_JMP      ||
            instr.op == OP_JZ       ||
            instr.op == OP_LOOP_COND ||
            instr.op == OP_BREAK    ||
            instr.op == OP_CONTINUE ||
            instr.op == OP_WHEN_START ||
            instr.op == OP_WHEN_END  ||
            instr.op == OP_THEN_END) {
            instr.operand += offset;
        }
        if (instr.op == OP_EXE_CHAIN) {
    instr.line += offset;
    if (instr.outerDepth >= 0) instr.outerDepth += offset;   
}
        if (instr.op == OP_THEN_CHECK) {
            instr.line += offset;
        }
    }
}

bytecode.push_back({OP_HALT, 0});


std::vector<Instruction> result;
result.insert(result.end(), functionsBytecode.begin(), functionsBytecode.end());
result.insert(result.end(), bytecode.begin(), bytecode.end());
return result;
}