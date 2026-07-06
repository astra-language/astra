/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include <string>
#include <vector>

struct Token {
    TokenType type;
    std::string value;
    long long intValue = 0;
    double floatValue = 0.0;
    TriState tristateValue= AST_FALSE;
    int line; 
};

class Lexer {
    std::string source;
    int pos;
    int line = 1;
public:
    Lexer(const std::string& src) : source(src), pos(0) {}
    
    
    void setLine(int l) { line = l; }
    
    std::vector<Token> tokenize();
};

#endif