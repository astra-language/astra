/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef ERROR_H
#define ERROR_H

#include <string>
#include "common.h"
#include <map>

enum class ErrCode {
     NONE,
    // Runtime errors
    UNDEFINED_VAR,
    UNINITIALIZED_VAR,
    STACK_UNDERFLOW,
    DIVISION_BY_ZERO,
    FUNC_NOT_FOUND,
    FIELD_NOT_FOUND,
    CHAIN_NOT_FOUND,
    MEMORY_OVERFLOW,

    // Syntax errors
    MISSING_SEMICOLON,
    MISSING_PAREN,
    MISSING_OPERATOR,
    INVALID_SYNTAX,
    UNEXPECTED_TOKEN,

    // Type errors
    TYPE_MISMATCH,
    INVALID_OPERATION,
    ALIAS_REQUIRED,

    INVALID_TIME_UNIT,
    INVALID_DATE_UNIT,

    NET_REQUEST_FAILED,
    NET_FILE_ERROR,
    NET_INIT_FAILED,

    MATH_DOMAIN_ERROR,     
    MATH_DIV_ZERO          
};

struct ErrorInfo {
    std::string title;
    std::string suggestion;
};


extern std::map<ErrCode, ErrorInfo> errorDatabase;


class AstraVM;



class AstraError {
public:
    static void setVM(AstraVM* vm);
    static void runtime(ErrCode code, int line = 0, const std::string& name = "");
    static void syntax(ErrCode code, int line = 0, const std::string& detail = "");
    static void type(ErrCode code, int line = 0, const std::string& detail = "");

     
    static ErrCode fromString(const std::string& s) {
        if (s == "UNDEFINED_VAR")    return ErrCode::UNDEFINED_VAR;
        if (s == "UNINITIALIZED_VAR") return ErrCode::UNINITIALIZED_VAR;
        if (s == "DIVISION_BY_ZERO") return ErrCode::DIVISION_BY_ZERO;
        if (s == "FUNC_NOT_FOUND")   return ErrCode::FUNC_NOT_FOUND;
        if (s == "FIELD_NOT_FOUND")  return ErrCode::FIELD_NOT_FOUND;
        if (s == "CHAIN_NOT_FOUND")  return ErrCode::CHAIN_NOT_FOUND;
        if (s == "MISSING_SEMICOLON") return ErrCode::MISSING_SEMICOLON;
        if (s == "INVALID_SYNTAX")   return ErrCode::INVALID_SYNTAX;
        if (s == "TYPE_MISMATCH")    return ErrCode::TYPE_MISMATCH;
        if (s == "INVALID_OPERATION") return ErrCode::INVALID_OPERATION;
        return ErrCode::NONE; 
    }

private:
    static std::string getMessage(ErrCode code, const std::string& detail);
};

#endif