/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "lexer.h"
#include <vector>

class Parser {
    const std::vector<Token>& tokens;
    size_t pos = 0;
  
std::string positionToVarName(int pos) {
    
    int suffix = (pos - 1) / 26;       
    int letterIdx = (pos - 1) % 26;    
    char letter = 'a' + letterIdx;
    if (suffix == 0) return std::string(1, letter);
    return std::string(1, letter) + std::to_string(suffix);
}


int varNameToPosition(const std::string& name) {
    char letter = name[0];
    int letterIdx = letter - 'a'; 
    int suffix = 0;
    if (name.size() > 1) {
        suffix = std::stoi(name.substr(1)); 
    }
    return suffix * 26 + letterIdx + 1;
}






public:

    bool hasError = false; 
    Parser(const std::vector<Token>& t) : tokens(t) {}

    ASTNode* parseWhenStatement();

    ASTNode* parseMultiAssignment();
    
    
    ASTNode* parseStatement();
    ASTNode* parseIfStatement(bool isChained = false);
    ASTNode* parseRepeatStatement();
    
    
    void parseBlock(ASTNode*& head);
    
   
    ASTNode* parseComparison();
    ASTNode* parseExpression();
    ASTNode* parseTerm();
    ASTNode* parsePower();
    ASTNode* parseFactor();

    ASTNode* makeErrorNode(int line);
    ASTNode* parseFunctionCall(const std::string& funcName);

    
    bool isIfLineValid(const std::string& line);
    bool isBlockLineValid(const std::string& line);
    
    void skipBlock();

    ASTNode* parseFuncDef();
    ASTNode* parseFuncCall();  
    ASTNode* parseReturn();

    ASTNode* parseModifierDef();
    ASTNode* parseExeStatement();

    ASTNode* parseChainDef();
    ASTNode* parseChainFieldStore();

    ASTNode* parseCheckStatement();
    ASTNode* parseCheckBody();  
    
    Token peek();
    Token consume();
    Token consume(TokenType type);
    Token consume(TokenType type, const std::string& errorMessage);

    void checkNoDoubleSemicolon(int line);
    
    size_t getCurrentPos() { return pos; }
    
    Token peekAt(size_t index) { 
        return (index < tokens.size()) ? tokens[index] : tokens.back(); 
    }
};

#endif