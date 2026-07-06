/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "error.h"
#include <iostream>
#include "vm.h" 


static AstraVM* g_currentVM = nullptr;


void AstraError::setVM(AstraVM* vm) {
    g_currentVM = vm;
}

std::map<ErrCode, ErrorInfo> errorDatabase = {
    {ErrCode::UNDEFINED_VAR,     {"UNDEFINED_VAR", "Check if the variable is declared"}},
    {ErrCode::UNINITIALIZED_VAR, {"Initialization Error", "Variable used before assignment"}},
    {ErrCode::STACK_UNDERFLOW,   {"Stack Error", "Unexpected empty stack"}},
    {ErrCode::DIVISION_BY_ZERO,  {"Math Error", "Denominator cannot be zero"}},
    {ErrCode::FUNC_NOT_FOUND,    {"Function Error", "Function not found"}},
    {ErrCode::FIELD_NOT_FOUND,   {"Field Error", "Field not found"}},
    {ErrCode::CHAIN_NOT_FOUND,   {"Chain Error", "Chain variable not found"}},
    {ErrCode::MEMORY_OVERFLOW,   {"Memory Error", "Memory limit exceeded"}},
    {ErrCode::MISSING_SEMICOLON, {"Syntax Error", "Add a ';' at the end of the statement"}},
    {ErrCode::MISSING_PAREN,     {"Syntax Error", "Missing parentheses"}},
    {ErrCode::MISSING_OPERATOR,  {"MISSING_OPERATOR", "Expected operator"}},
    {ErrCode::INVALID_SYNTAX,    {"INVALID_SYNTAX", "Invalid syntax detected"}},
    {ErrCode::UNEXPECTED_TOKEN,  {"Syntax Error", "Unexpected token"}},
    {ErrCode::TYPE_MISMATCH,     {"Type Error", "Ensure the data types are compatible"}},
    {ErrCode::INVALID_OPERATION, {"INVALID_OPERATION", "Operation not supported"}},
    {ErrCode::ALIAS_REQUIRED,   {"ALIAS_REQUIRED", "Function is attached with alias — use alias.function()"}},
    {ErrCode::INVALID_TIME_UNIT, {"TIME_ERROR", "Invalid time unit (use h, m, s)"}},
    {ErrCode::INVALID_DATE_UNIT, {"DATE_ERROR", "Invalid date unit (use d, w, m, y)"}},
    {ErrCode::NET_REQUEST_FAILED, {"Network Error", "Check the URL or your internet connection"}},
    {ErrCode::NET_FILE_ERROR,     {"File Error", "Check the file path and write permissions"}},
    {ErrCode::NET_INIT_FAILED,    {"Network Error", "Could not initialize network handle"}},
    {ErrCode::MATH_DOMAIN_ERROR, {"Math Domain Error", "Input is out of valid range for this function"}},
    {ErrCode::MATH_DIV_ZERO,     {"Math Error", "Cannot divide/modulo by zero"}}
};

std::string AstraError::getMessage(ErrCode code, const std::string& detail) {
    if (errorDatabase.find(code) != errorDatabase.end()) {
        ErrorInfo info = errorDatabase[code];
        return info.title + ": " + detail + ". " + YELLOW + "(" + info.suggestion + ")" + RESET;
    }
    return "Unknown Error occurred.";
}

void AstraError::runtime(ErrCode code, int line, const std::string& name) {

    
    if (g_currentVM && g_currentVM->inWhenBlock) {
        g_currentVM->hasRuntimeError = true;
        g_currentVM->lastErrCode     = code;
        return;
    }

    std::cerr << RED << "[ASTRA-RUN-ERR]" << RESET;
    if (line > 0)
        std::cerr << " :: " << YELLOW << "Line " << line << RESET;
    
    
    std::string coloredName = CYAN + name + RESET;
    std::cerr << " :: " << getMessage(code, coloredName) << "\n" << std::flush;
}

void AstraError::syntax(ErrCode code, int line, const std::string& detail) {
    std::cerr << RED << "[ASTRA-SYN-ERR]" << RESET;
    if (line > 0)
        std::cerr << " :: " << YELLOW << "Line " << line << RESET;
        std::string coloredName = CYAN + detail + RESET;
    std::cerr << " :: " << getMessage(code, coloredName) << std::endl;
}

void AstraError::type(ErrCode code, int line, const std::string& detail) {
    std::cerr << RED << "[ASTRA-TYPE-ERR]" << RESET;
    if (line > 0)
        std::cerr << " :: " << YELLOW << "Line " << line << RESET;
    std::cerr << " :: " << getMessage(code, detail) << std::endl;
}