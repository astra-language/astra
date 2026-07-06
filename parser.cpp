/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "parser.h"
#include <iostream>
#include "powermanager.h"
#include "error.h"
#include <set>

void Parser::parseBlock(ASTNode*& head) {
    ASTNode* current = nullptr;
    while (peek().type != TOKEN_ELSE && peek().type != TOKEN_EOF) {
        if (peek().type == TOKEN_SEMICOLON) break;
        ASTNode* stmt = parseStatement();
        if (!stmt) break;
        if (!head) { head = stmt; current = head; }
        else { current->right = stmt; current = current->right; }
    }
    
    if (peek().type == TOKEN_EOF && head != nullptr) {
        AstraError::syntax(ErrCode::MISSING_SEMICOLON, peek().line, "");
        hasError = true; 
    }
}

ASTNode* Parser::parseFunctionCall(const std::string& funcName) {
    if (PowerManager::getInstance().registry.count(funcName) == 0) {
        ASTNode* callNode = new ASTNode();
        callNode->type = NODE_FUNC_CALL;
        callNode->varName = funcName;
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            callNode->arguments.push_back(parseExpression());
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume();
        return callNode;
    }
    ASTNode* node = new ASTNode();
    node->type = NODE_POWER_CALL;
    node->varName = funcName;
    node->lineNumber = peek().line;
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        node->arguments.push_back(parseExpression());
        if (peek().type == TOKEN_COMMA) consume();
        else if (peek().type != TOKEN_RPAREN) break;
    }
    if (peek().type == TOKEN_RPAREN) consume();
    return node;
}

ASTNode* Parser::parseFuncDef() {
    consume(); 
    Token nameToken = consume(TOKEN_VAR, "Expected function name after #f");
    ASTNode* node = new ASTNode();
    node->type = NODE_FUNC_DEF;
    node->varName = nameToken.value;
    node->lineNumber = nameToken.line;
    consume(TOKEN_LPAREN, "Expected '(' after function name");
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        Token param = consume(TOKEN_VAR, "Expected parameter name");
        node->params.push_back(param.value);
        if (peek().type == TOKEN_COMMA) consume();
    }
    consume(TOKEN_RPAREN, "Expected ')' after parameters");
    ASTNode* bodyHead = nullptr;
    ASTNode* current = nullptr;
    while (peek().type != TOKEN_FUNC_END && peek().type != TOKEN_EOF) {
        
        if (peek().type == TOKEN_SEMICOLON) {
            AstraError::syntax(ErrCode::INVALID_SYNTAX, peek().line,
                " Functions end with '#ef' not ';' (only chain methods use ';')");
            hasError = true;
            consume(); 
            break;
        }
        ASTNode* stmt = parseStatement();
        if (!stmt) continue;
        if (!bodyHead) { bodyHead = stmt; current = bodyHead; }
        else { current->right = stmt; current = current->right; }
    }
    if (peek().type == TOKEN_FUNC_END) {
        consume(); 
    } else {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, nameToken.line,
            " Missing '#ef' to close function '" + node->varName + "'");
        hasError = true;
    }
    node->body = bodyHead;
    return node;
}

void Parser::checkNoDoubleSemicolon(int line) {
    if (peek().type == TOKEN_SEMICOLON) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, line,
            "Statements must NOT end with ';'. "
            "Only the block itself (if/repeat/check/when) ends with one ';'");
        hasError = true;
    }
}

ASTNode* Parser::parseExeStatement() {
    consume(); 
    consume(TOKEN_LPAREN, "Expected '(' after exe");
    
    ASTNode* node = new ASTNode();
    node->type = NODE_EXE;
    node->lineNumber = peek().line;
    
    if (peek().type == TOKEN_CHAIN_DEF) {
        Token chainTok = consume();
        node->varName = chainTok.value;
        node->isChainExe = true;
    }
    else if (peek().type == TOKEN_LPAREN) {
        consume();
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            Token varTok = consume(TOKEN_VAR, "Expected variable name");
            node->params.push_back(varTok.value);
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume();
        node->varName = "";
    }
    else {
        Token varTok = consume(TOKEN_VAR, "Expected variable name");
        node->varName = varTok.value;
        node->params.push_back(varTok.value);
    }
    
    consume(TOKEN_COMMA, "Expected ','");
    node->left = parseComparison(); 

    
    if (peek().type == TOKEN_COMMA) {
        consume();
        node->condition = parseComparison();   
    }

    consume(TOKEN_RPAREN, "Expected ')'");
    
    return node;
}

ASTNode* Parser::parseModifierDef() {
    consume(); 
    Token nameToken = consume(TOKEN_VAR, "Expected modifier name after #m");
    ASTNode* node = new ASTNode();
    node->type = NODE_MODIFIER_DEF;
    node->varName = nameToken.value;
    node->lineNumber = nameToken.line;
    consume(TOKEN_LPAREN, "Expected '(' after modifier name");
    consume(TOKEN_RPAREN, "Expected ')'");

   
    ASTNode* beforeBody = nullptr;
    if (peek().type == TOKEN_MOD_BEFORE) {
        consume(); 
        while (peek().type != TOKEN_MOD_AFTER &&
               peek().type != TOKEN_FUNC_END &&
               peek().type != TOKEN_EOF) {
            ASTNode* stmt = parseStatement();
            if (!stmt) continue;
            if (!beforeBody) beforeBody = stmt;
            else {
                ASTNode* cur = beforeBody;
                while (cur->right) cur = cur->right;
                cur->right = stmt;
            }
        }
    }
    node->body = beforeBody; 

    
    ASTNode* afterBody = nullptr;
    if (peek().type == TOKEN_MOD_AFTER) {
        consume(); 
        while (peek().type != TOKEN_FUNC_END &&
               peek().type != TOKEN_EOF) {
            ASTNode* stmt = parseStatement();
            if (!stmt) continue;
            if (!afterBody) afterBody = stmt;
            else {
                ASTNode* cur = afterBody;
                while (cur->right) cur = cur->right;
                cur->right = stmt;
            }
        }
    }
    node->condition = afterBody; 

    consume(TOKEN_FUNC_END, "Expected '#ef' to close modifier");
    return node;
}

ASTNode* Parser::parseFuncCall() {
    Token nameToken = consume();
    consume();
    ASTNode* node = new ASTNode();
    node->type = NODE_FUNC_CALL;
    node->varName = nameToken.value;
    node->lineNumber = nameToken.line;
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        node->arguments.push_back(parseExpression());
        if (peek().type == TOKEN_COMMA) consume();
    }
    consume();
    return node;
}

ASTNode* Parser::parseReturn() {
    Token t = consume();
    ASTNode* node = new ASTNode();
    node->type = NODE_RETURN;
    node->lineNumber = t.line;
    node->left = parseComparison();
    return node;
}

Token Parser::peek() {
    if (pos >= tokens.size()) return {TOKEN_EOF, "", 0, 0.0, AST_FALSE};
    return tokens[pos];
}

Token Parser::consume() { return tokens[pos++]; }

Token Parser::consume(TokenType type) {
    if (peek().type == type) return tokens[pos++];
    
    if (type == TOKEN_TO)
        AstraError::syntax(ErrCode::MISSING_OPERATOR, peek().line, ": 'to' keyword missing");
    else
        AstraError::syntax(ErrCode::UNEXPECTED_TOKEN, peek().line, peek().value);
    return {TOKEN_ERROR, "Unexpected token", 0, 0.0, AST_FALSE, peek().line};
}

Token Parser::consume(TokenType type, const std::string& errorMessage) {
    if (peek().type == type) return tokens[pos++];
    AstraError::syntax(ErrCode::INVALID_SYNTAX, peek().line, errorMessage);
    return {TOKEN_ERROR, "Error", 0, 0.0, AST_FALSE, peek().line};
}

bool Parser::isIfLineValid(const std::string& line) {
    Lexer lexer(line);
    auto tokens = lexer.tokenize();
    if (tokens.size() < 3) return false;
    if (tokens[0].type != TOKEN_IF) return false;
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].type == TOKEN_GT  || tokens[i].type == TOKEN_LT  ||
            tokens[i].type == TOKEN_EQ  || tokens[i].type == TOKEN_NEQ ||
            tokens[i].type == TOKEN_GTE || tokens[i].type == TOKEN_LTE)
            return true;
    }
    return false;
}

bool Parser::isBlockLineValid(const std::string& line) {
    Lexer lexer(line);
    auto tokens = lexer.tokenize();
    if (tokens.size() < 3) return false;
    if (tokens[0].type == TOKEN_IF) {
        for (size_t i = 1; i < tokens.size(); i++) {
            if (tokens[i].type == TOKEN_GT  || tokens[i].type == TOKEN_LT  ||
                tokens[i].type == TOKEN_EQ  || tokens[i].type == TOKEN_NEQ ||
                tokens[i].type == TOKEN_GTE || tokens[i].type == TOKEN_LTE) {
                
                if (i + 1 >= tokens.size() - 1) return false; 
                return true;
            }
        }
        return false;
    }
    if (tokens[0].type == TOKEN_REPEAT) return true;
    return false;
}


ASTNode* Parser::parseCheckBody() {
    ASTNode* body = nullptr;
    while (peek().type != TOKEN_SEMICOLON &&
           peek().type != TOKEN_EOF &&
           peek().type != TOKEN_NUMBER &&
           peek().type != TOKEN_STRING &&
           peek().type != TOKEN_END) {
        ASTNode* stmt = parseStatement();
        if (!stmt) break;
        if (!body) body = stmt;
        else {
            ASTNode* cur = body;
            while (cur->right) cur = cur->right;
            cur->right = stmt;
        }
    }
    return body;
}

ASTNode* Parser::parseCheckStatement() {
    consume(); 
    ASTNode* node = new ASTNode();
    node->type = NODE_CHECK;
    node->lineNumber = peek().line;
    node->left = parseComparison(); 

    while (peek().type != TOKEN_SEMICOLON && peek().type != TOKEN_EOF) {
        
        
        if (peek().type == TOKEN_END) {
            consume(); 
            ASTNode* def = new ASTNode();
            def->type = NODE_LITERAL;
            def->value.type = VAL_STR;
            def->value.str = "__default__";
            def->left = parseCheckBody();
            node->arguments.push_back(def);
            break;
        }
        
        else if (peek().type == TOKEN_NUMBER) {
            Token numTok = consume();
            if (peek().type == TOKEN_MINUS) {
                consume(); 
                Token endTok = consume(TOKEN_NUMBER, "Expected range end");
                consume(); 
                ASTNode* rangeCase = new ASTNode();
                rangeCase->type = NODE_LITERAL;
                rangeCase->value.type = VAL_STR;
                rangeCase->value.str = "__range__";
                rangeCase->value.num = numTok.intValue;
                ASTNode* endNode = new ASTNode();
                endNode->type = NODE_LITERAL;
                endNode->value.type = VAL_INT;
                endNode->value.num = endTok.intValue;
                rangeCase->right = endNode;
                rangeCase->left = parseCheckBody();
                node->arguments.push_back(rangeCase);
            } else {
                consume(); 
                ASTNode* intCase = new ASTNode();
                intCase->type = NODE_LITERAL;
                intCase->value.type = VAL_INT;
                intCase->value.num = numTok.intValue;
                intCase->left = parseCheckBody();
                node->arguments.push_back(intCase);
            }
        }
        
        else if (peek().type == TOKEN_STRING) {
            Token strTok = consume();
            consume(); 
            ASTNode* strCase = new ASTNode();
            strCase->type = NODE_LITERAL;
            strCase->value.type = VAL_STR;
            strCase->value.str = strTok.value;
            strCase->left = parseCheckBody();
            node->arguments.push_back(strCase);
        }
        else break;
    }
    if (peek().type == TOKEN_SEMICOLON) {
    consume();
}
    return node;
}

ASTNode* Parser::parseFactor() {
    
    Token t = consume();
    if (t.type == TOKEN_EOF) return nullptr;

    ASTNode* node = new ASTNode();
    node->lineNumber = t.line;

    if (t.type == TOKEN_USER_INPUT) {
        node->type = NODE_USER_INPUT;
        node->op   = OP_USER_INPUT;
        Token prompt = consume(TOKEN_PROMPT_TEXT);
        node->value.type = VAL_STR;
        node->value.str  = prompt.value;
    }
    else if (t.type == TOKEN_EXE) {
    pos--; 
    return parseExeStatement();
}
    else if (t.type == TOKEN_ALIAS_CALL) {
    if (peek().type == TOKEN_LPAREN) {
        
        node->type = NODE_FUNC_CALL;
        node->varName = t.value;
        consume(); 
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            node->arguments.push_back(parseExpression());
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume(); 
    } 
    else if (peek().type == TOKEN_LBRACKET) {
        
        consume(); 
        ASTNode* indexExpr = parseExpression();
        consume(); 
        ASTNode* dynNode = new ASTNode();
        dynNode->type    = NODE_CHAIN_DYNAMIC;
        dynNode->varName = t.value; 
        dynNode->lineNumber = t.line;
        dynNode->left    = indexExpr;
        delete node;
        return dynNode;
    } else {
        
        node->type    = NODE_VAR;
        node->varName = t.value; 
    }
    return node;
}
    else if (t.type == TOKEN_NUMBER) {
        node->type = NODE_LITERAL;
        node->value.type = VAL_INT;
        node->value.num  = t.intValue;
    }
    else if (t.type == TOKEN_FLOAT) {
        node->type = NODE_LITERAL;
        node->value.type    = VAL_FLOAT;
        node->value.decimal = t.floatValue;
    }
    else if (t.type == TOKEN_BOOL) {
        node->type = NODE_LITERAL;
        node->value.type     = VAL_BOOL;
        node->value.tristate = (TriState)t.tristateValue;
    }
    else if (t.type == TOKEN_STRING) {
        node->type = NODE_LITERAL;
        node->value.type = VAL_STR;
        node->value.str  = t.value;
    }
    else if (t.type == TOKEN_CHAIN_DEF) {
    
    if (peek().type == TOKEN_LBRACKET) {
        consume(); 
        ASTNode* indexExpr = parseExpression();
        consume(); 
        ASTNode* dynNode = new ASTNode();
        dynNode->type    = NODE_CHAIN_DYNAMIC;
        dynNode->varName = t.value; 
        dynNode->lineNumber = t.line;
        dynNode->left    = indexExpr;
        return dynNode;
    }
    
    node->type    = NODE_CHAIN_ACCESS;
    node->varName = t.value;
}
    else if (t.type == TOKEN_CHAIN_FIELD) {
        node->type    = NODE_CHAIN_FIELD_ACCESS;
        node->varName = t.value;

        while (peek().type == TOKEN_COLON) {
            consume(); 
            Token nextSeg = consume(); 
            node->varName += ":" + nextSeg.value;
        }
    }
    else if (t.type == TOKEN_CHAIN_FUNC) {
    consume(TOKEN_LPAREN, "Expected '(' after chain function");
    std::string funcName = t.value; 
    
    ASTNode* node = new ASTNode();
    node->type = NODE_CHAIN_FUNC;
    node->varName = funcName;
    node->lineNumber = t.line;
    
    
    Token chainTok = consume(); 
    node->params.push_back(chainTok.value); 
    
    
    if (funcName == "merge" && peek().type == TOKEN_COMMA) {
        consume(); 
        Token chain2 = consume();
        node->params.push_back(chain2.value); 
    }
    
else if (funcName == "self") {
    while (peek().type == TOKEN_COMMA) {
        consume(); 
        if (peek().type == TOKEN_LPAREN) {
            
            consume(); 
            while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                node->arguments.push_back(parseExpression());
                if (peek().type == TOKEN_COMMA) consume();
            }
            consume();
        } else {
            node->arguments.push_back(parseExpression());
        }
    }
}else if ((funcName == "contains" || funcName == "indexOf" || funcName == "join") 
             && peek().type == TOKEN_COMMA) {
        consume();
        node->arguments.push_back(parseExpression()); 
    }
    
    consume(TOKEN_RPAREN, "Expected ')'");
    return node;
}
    else if (t.type == TOKEN_VAR) {
        
    if (t.value == "adr" || t.value == "val") {
    ASTNode* node = new ASTNode();
    node->type = NODE_FUNC_CALL; 
    node->varName = t.value;
    node->lineNumber = t.line;
    
    consume(TOKEN_LPAREN); 
    
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        node->arguments.push_back(parseExpression());
        if (peek().type == TOKEN_COMMA) consume();
    }
    
    consume(TOKEN_RPAREN); 
    
    return node;
}
    if (t.value == "create" || t.value == "read" || t.value == "plus" ||
    t.value == "clear" || t.value == "remove" || t.value == "close" ||
    t.value == "eof" || t.value == "fetch" || t.value == "parseJson" ||
    t.value == "toJson" || t.value == "jsonpretty" ) {
        ASTNode* node = new ASTNode();
        node->type = NODE_FUNC_CALL;
        node->varName = t.value;
        node->lineNumber = t.line;

        consume(TOKEN_LPAREN);
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            node->arguments.push_back(parseExpression());
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume(TOKEN_RPAREN);

        return node;
    }
        
if (peek().type == TOKEN_LBRACKET) {
    consume(); 
    ASTNode* indexExpr = parseExpression();
    consume(); 
    
    
    if (peek().type == TOKEN_COLON) {
        consume(); 
        std::string fieldName = consume().value; 
        
        ASTNode* dynNode = new ASTNode();
        dynNode->type    = NODE_CHAIN_DYNAMIC_FIELD_LOAD; 
        dynNode->varName = t.value; 
        dynNode->lineNumber = t.line;
        dynNode->left    = indexExpr;
        dynNode->params.push_back(fieldName);
        delete node;
        return dynNode;
    }
    
    ASTNode* dynNode = new ASTNode();
    dynNode->type    = NODE_CHAIN_DYNAMIC;
    dynNode->varName = t.value;
    dynNode->lineNumber = t.line;
    dynNode->left    = indexExpr;
    delete node;
    return dynNode;
}
        
        else if (peek().type == TOKEN_LPAREN) {
            delete node;
            consume(); 
            if (PowerManager::getInstance().registry.count(t.value) == 0) {
                ASTNode* callNode = new ASTNode();
                callNode->type       = NODE_FUNC_CALL;
                callNode->varName    = t.value;
                callNode->lineNumber = t.line;
                while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                    callNode->arguments.push_back(parseExpression());
                    if (peek().type == TOKEN_COMMA) consume();
                }
                consume(); 
                return callNode;
            } else {
                ASTNode* pnode = new ASTNode();
                pnode->type    = NODE_POWER_CALL;
                pnode->varName = t.value;
                pnode->lineNumber = t.line;
                while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                    pnode->arguments.push_back(parseExpression());
                    if (peek().type == TOKEN_COMMA) consume();
                }
                consume(); 
                return pnode;
            }
        }
        else {
            node->type    = NODE_VAR;
            node->varName = t.value;
        }
    }
    else if (t.type == TOKEN_LPAREN) {
        delete node;
        node = parseComparison();
        consume(); 
    }
    else if (t.type == TOKEN_MINUS) {
        ASTNode* neg = new ASTNode();
        neg->type       = NODE_NEGATE;
        neg->lineNumber = t.line;
        neg->left       = parseFactor();
        delete node;
        return neg;
    }
    else if (t.type == TOKEN_NOT) {
        ASTNode* notNode = new ASTNode();
        notNode->type = NODE_NOT;
        notNode->left = parseFactor();
        delete node;
        return notNode;
    }
    else {
        delete node;
        return nullptr;
    }
    return node;
}

ASTNode* Parser::parsePower() {
    ASTNode* node = parseFactor();

    while (peek().type == TOKEN_POW) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        Token op = consume();
        opNode->op = OP_POW;
        opNode->left = node;
        opNode->right = parseFactor();

        if (opNode->right == nullptr) {   /* ← కొత్త చెక్ */
            AstraError::syntax(ErrCode::MISSING_OPERATOR, op.line,
                              "Expected expression after '^'");
            hasError = true;
            return opNode->left;
        }

        node = opNode;
    }
    return node;
}

ASTNode* Parser::parseTerm() {
    ASTNode* node = parsePower();
    while (peek().type == TOKEN_STAR || peek().type == TOKEN_SLASH || peek().type == TOKEN_MOD) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        Token op = consume();
        if      (op.type == TOKEN_STAR)  opNode->op = OP_MUL;
        else if (op.type == TOKEN_SLASH) opNode->op = OP_DIV;
        else                             opNode->op = OP_MOD;
        opNode->left  = node;
        opNode->right = parsePower();

        if (opNode->right == nullptr) {   /* ← కొత్త చెక్ */
            std::string opSym = (op.type == TOKEN_STAR) ? "*" :
                                 (op.type == TOKEN_SLASH) ? "/" : "%";
            AstraError::syntax(ErrCode::MISSING_OPERATOR, op.line,
                              "Expected expression after '" + opSym + "'");
            hasError = true;
            return opNode->left;
        }

        node = opNode;
    }
    return node;
}

ASTNode* Parser::parseExpression() {
    ASTNode* node = parseTerm();
    while (peek().type == TOKEN_PLUS || peek().type == TOKEN_MINUS) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        Token opTok = consume();
        opNode->op   = (opTok.type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        opNode->left  = node;
        opNode->right = parseTerm();
        
        if (opNode->right == nullptr) {   
            AstraError::syntax(ErrCode::MISSING_OPERATOR, opTok.line, 
                              "Expected expression after '+' or '-'");
            hasError = true;
            return opNode->left;   
        }
        
        node = opNode;
    }
    return node;
}

ASTNode* Parser::parseComparison() {
    ASTNode* node = parseExpression();
    while (peek().type == TOKEN_GT  || peek().type == TOKEN_LT  ||
           peek().type == TOKEN_EQ  || peek().type == TOKEN_NEQ ||
           peek().type == TOKEN_GTE || peek().type == TOKEN_LTE) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        Token opToken = consume();
        if      (opToken.type == TOKEN_GT)  opNode->op = OP_GT;
        else if (opToken.type == TOKEN_LT)  opNode->op = OP_LT;
        else if (opToken.type == TOKEN_EQ)  opNode->op = OP_EQ;
        else if (opToken.type == TOKEN_NEQ) opNode->op = OP_NEQ;
        else if (opToken.type == TOKEN_GTE) opNode->op = OP_GTE;
        else                                opNode->op = OP_LTE;
        opNode->left  = node;
        opNode->right = parseExpression();
        node = opNode;
    }
    while (peek().type == TOKEN_AND) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        opNode->op   = OP_AND;
        consume();
        opNode->left  = node;
        opNode->right = parseExpression();
        node = opNode;
    }
    while (peek().type == TOKEN_OR) {
        ASTNode* opNode = new ASTNode();
        opNode->lineNumber = peek().line;
        opNode->type = NODE_OP;
        opNode->op   = OP_OR;
        consume();
        opNode->left  = node;
        opNode->right = parseExpression();
        node = opNode;
    }
    return node;
}

void Parser::skipBlock() {
    while (peek().type != TOKEN_SEMICOLON && peek().type != TOKEN_EOF) consume();
    if (peek().type == TOKEN_SEMICOLON) consume();
}

ASTNode* Parser::makeErrorNode(int line) {
    ASTNode* err = new ASTNode();
    err->type = NODE_ERROR;
    err->lineNumber = line;
    return err;
}

ASTNode* Parser::parseRepeatStatement() {
    Token t = consume(TOKEN_REPEAT, "Error: Expected 'repeat' keyword.");
    ASTNode* node = new ASTNode();
    node->lineNumber = t.line;

    if (peek().type == TOKEN_LPAREN) {
        node->type = NODE_REPEAT_COND;
        consume(TOKEN_LPAREN);
        node->condition = parseComparison();
        if (node->condition->type != NODE_OP ||
            (node->condition->op != OP_GT  && node->condition->op != OP_LT  &&
             node->condition->op != OP_EQ  && node->condition->op != OP_NEQ &&
             node->condition->op != OP_GTE && node->condition->op != OP_LTE)) {
            AstraError::syntax(ErrCode::MISSING_OPERATOR, t.line, "condition operator");
            skipBlock(); return makeErrorNode(t.line);
        }
        if (peek().type != TOKEN_RPAREN) {
            AstraError::syntax(ErrCode::MISSING_PAREN, t.line, "repeat condition");
            skipBlock(); return makeErrorNode(t.line);
        }
        consume(TOKEN_RPAREN);
    }
    else if (peek().type == TOKEN_VAR && peekAt(pos + 1).type == TOKEN_TO) {
        node->type = NODE_REPEAT;
        node->iteratorName = consume(TOKEN_VAR).value;
        consume(TOKEN_TO);
        if (peek().type == TOKEN_LPAREN) {
            consume(TOKEN_LPAREN);
            node->startExpr = parseExpression();
            if (peek().type == TOKEN_COMMA) { consume(); node->endExpr = parseExpression(); }
            if (peek().type == TOKEN_COMMA) { consume(); node->stepExpr = parseExpression(); }
            else {
                node->stepExpr = new ASTNode();
                node->stepExpr->type = NODE_LITERAL;
                node->stepExpr->value.type = VAL_INT;
                node->stepExpr->value.num  = 1;
            }
            consume(TOKEN_RPAREN);
        } else {
            node->startExpr = new ASTNode();
            node->startExpr->type = NODE_LITERAL;
            node->startExpr->value.type = VAL_INT;
            node->startExpr->value.num  = 1;

            
        if (peek().type == TOKEN_EOF || 
            peek().type == TOKEN_SEMICOLON ||
            peek().type == TOKEN_WRITE ||
            peek().type == TOKEN_IF ||
            peek().type == TOKEN_REPEAT) {
            AstraError::syntax(ErrCode::MISSING_OPERATOR, t.line, 
                             "limit value after 'to'");
            skipBlock(); return makeErrorNode(t.line);
        }


            node->endExpr = parseExpression();
            node->stepExpr = new ASTNode();
            node->stepExpr->type = NODE_LITERAL;
            node->stepExpr->value.type = VAL_INT;
            node->stepExpr->value.num  = 1;
        }
    }
    
else if (peek().type == TOKEN_VAR && peekAt(pos + 1).type != TOKEN_TO) {
    consume(); 
    AstraError::syntax(ErrCode::MISSING_OPERATOR, t.line, 
                      "'to' keyword missing after iterator");
    skipBlock(); return makeErrorNode(t.line);
}
    else {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, t.line, " after 'repeat'");
        skipBlock(); return makeErrorNode(t.line);
    }

    parseBlock(node->body);

    if (peek().type == TOKEN_SEMICOLON) {
        consume(TOKEN_SEMICOLON);
    } else {
        AstraError::syntax(ErrCode::MISSING_SEMICOLON, t.line, "");
        skipBlock(); return makeErrorNode(t.line);
    }
    return node;
}

ASTNode* Parser::parseWhenStatement() {
    Token t = consume(); 

    
    if (peek().type != TOKEN_COLON) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, t.line,
                          " Expected ':' after when");
        hasError = true;
        while (peek().type != TOKEN_SEMICOLON && peek().type != TOKEN_EOF) consume();
        if (peek().type == TOKEN_SEMICOLON) consume();
        return nullptr;
    }
    consume(TOKEN_COLON); 


    ASTNode* node = new ASTNode();
    node->type = NODE_WHEN;
    node->lineNumber = t.line;

    
    ASTNode* bodyHead = nullptr;
    while (peek().type != TOKEN_THEN &&
           peek().type != TOKEN_SEMICOLON &&
           peek().type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement();
        if (!stmt) continue;
        if (!bodyHead) bodyHead = stmt;
        else {
            ASTNode* cur = bodyHead;
            while (cur->right) cur = cur->right;
            cur->right = stmt;
        }
    }
    node->body = bodyHead;

    
    while (peek().type == TOKEN_THEN) {
        consume(); 

        ASTNode* thenNode = new ASTNode();
        thenNode->type = NODE_THEN;

       
        if (peek().type == TOKEN_VAR) {
            thenNode->varName = consume().value;
        }
        
    if (peek().type != TOKEN_COLON) {
    AstraError::syntax(ErrCode::INVALID_SYNTAX, peek().line, 
                      " Expected ':' after then");
    hasError = true; 
    while (peek().type != TOKEN_SEMICOLON && peek().type != TOKEN_EOF) consume();
    if (peek().type == TOKEN_SEMICOLON) consume();
    return node;
}
consume(TOKEN_COLON);
        
        ASTNode* thenBody = nullptr;
        while (peek().type != TOKEN_THEN &&
               peek().type != TOKEN_SEMICOLON &&
               peek().type != TOKEN_EOF) {
            ASTNode* stmt = parseStatement();
            if (!stmt) continue;
            if (!thenBody) thenBody = stmt;
            else {
                ASTNode* cur = thenBody;
                while (cur->right) cur = cur->right;
                cur->right = stmt;
            }
        }
        thenNode->body = thenBody;
        node->arguments.push_back(thenNode);

        
        if (peek().type == TOKEN_SEMICOLON) {
    consume();
    break;
}
        else if (peek().type == TOKEN_EOF) {
        AstraError::syntax(ErrCode::MISSING_SEMICOLON, t.line, "");
        hasError = true;
        break;
    }
    }

    return node;
}

ASTNode* Parser::parseMultiAssignment() {
    std::vector<std::string> varNames;
    
    
    std::string firstVar = consume(TOKEN_VAR).value;
    
    
    if (peek().type == TOKEN_DECREMENT) {
    consume(); 
    std::string endVar = consume(TOKEN_VAR).value;
    
    int startPos = varNameToPosition(firstVar);
    int endPos   = varNameToPosition(endVar);
    
    for (int p = startPos; p <= endPos; p++) {
        varNames.push_back(positionToVarName(p));
    }
} else {
    varNames.push_back(firstVar);
}
    
    while (peek().type == TOKEN_COMMA) {
        consume();
        varNames.push_back(consume(TOKEN_VAR).value);
    }
    
    consume(TOKEN_ASSIGN, "Expected '=' in multi-assignment");
    
    
    std::vector<ASTNode*> values;
    ASTNode* firstVal = parseComparison(); 
    
    if (peek().type == TOKEN_DECREMENT && firstVal->type == NODE_LITERAL && firstVal->value.type == VAL_INT) {
        consume(); 
        ASTNode* endVal = parseComparison(); 
        
        long long start = firstVal->value.num;
        long long end = endVal->value.num;
        for (long long n = start; n <= end; n++) {
            ASTNode* litNode = new ASTNode();
            litNode->type = NODE_LITERAL;
            litNode->value.type = VAL_INT;
            litNode->value.num = n;
            values.push_back(litNode);
        }
    } else {
        values.push_back(firstVal);
    }
    
    while (peek().type == TOKEN_COMMA) {
        consume();
        values.push_back(parseComparison());
    }
    
    
    if (values.size() == 1 && varNames.size() > 1) {
    ASTNode* singleVal = values[0];
    values.clear();
    for (size_t i = 0; i < varNames.size(); i++) {
        values.push_back(singleVal);
    }
}
else if (varNames.size() != values.size()) {
    AstraError::syntax(ErrCode::INVALID_SYNTAX, 0,
        " Multi-assign count mismatch: " + std::to_string(varNames.size()) +
        " variables but " + std::to_string(values.size()) + " values");
    return nullptr;
}
    
    
    ASTNode* head = nullptr;
    ASTNode* current = nullptr;
    for (size_t i = 0; i < varNames.size(); i++) {
        ASTNode* storeNode = new ASTNode();
        storeNode->type = NODE_OP;
        storeNode->op = OP_STORE;
        storeNode->varName = varNames[i];
        storeNode->left = values[i];
        
        if (!head) { head = storeNode; current = head; }
        else { current->right = storeNode; current = current->right; }
    }
    
    return head;
}

ASTNode* Parser::parseStatement() {
    Token t = peek();

    
if (t.type == TOKEN_DEALIAS) {
    consume();
    Token nameTok = consume(TOKEN_VAR, "Expected variable name after 'dealias'");
    ASTNode* node = new ASTNode();
    node->type    = NODE_DEALIAS;
    node->varName = nameTok.value;  
    node->lineNumber = t.line;
    return node;
}
    if (t.type == TOKEN_ATTACH) {
        consume(); 
        
        Token pathTok = consume(TOKEN_STRING, "Expected file path after 'attach'");
        
        ASTNode* node = new ASTNode();
        node->type = NODE_ATTACH;
        node->lineNumber = t.line;
        node->varName = pathTok.value; 
        
        
        if (peek().type == TOKEN_AS) {
            consume(); 
            Token aliasTok = consume(TOKEN_VAR, "Expected alias name after 'as'");
            node->params.push_back(aliasTok.value); 
        }
        
        return node;
    }
    if (t.type == TOKEN_INCLUDE) {
    consume();

    Token pathTok = consume(TOKEN_STRING, "Expected file path after 'include'");

    ASTNode* node = new ASTNode();
    node->type = NODE_INCLUDE;
    node->lineNumber = t.line;
    node->varName = pathTok.value;
    node->value.type = VAL_STR;
    node->value.str = "";

    if (peek().type == TOKEN_ONLY) {
        consume();
        node->value.str = "only";
        node->params.push_back(consume(TOKEN_VAR, "Expected name after 'only'").value);
        while (peek().type == TOKEN_COMMA) {
            consume();
            node->params.push_back(consume(TOKEN_VAR, "Expected name after ','").value);
        }
    }
    else if (peek().type == TOKEN_EXCEPT) {
        consume();
        node->value.str = "except";
        node->params.push_back(consume(TOKEN_VAR, "Expected name after 'except'").value);
        while (peek().type == TOKEN_COMMA) {
            consume();
            node->params.push_back(consume(TOKEN_VAR, "Expected name after ','").value);
        }
    }

    return node;
}
    if (t.type == TOKEN_FUNC_START) return parseFuncDef();
    if (t.type == TOKEN_EXE) return parseExeStatement();
    if (t.type == TOKEN_MODIFIER_START) return parseModifierDef();
    if (t.type == TOKEN_IF)     return parseIfStatement();
    if (t.type == TOKEN_REPEAT) return parseRepeatStatement();
    if (t.type == TOKEN_RETURN) return parseReturn();
    if (t.type == TOKEN_CHECK) return parseCheckStatement();
    if (t.type == TOKEN_WHEN)  return parseWhenStatement();
    
if (t.type == TOKEN_CONST) {
    consume(); 

     
    if (peek().type == TOKEN_CHAIN_DEF) {
        AstraError::syntax(ErrCode::INVALID_SYNTAX, t.line, 
                          " Chains cannot be declared as const");
        
        while (peek().type != TOKEN_EOF && 
               peek().type != TOKEN_SEMICOLON) consume();
        return nullptr;
    }

    Token nameToken = consume(TOKEN_VAR, "Expected name after const");
    consume(TOKEN_ASSIGN, " Expected '=' after variable name");
    
    ASTNode* node = new ASTNode();
    node->type = NODE_OP;
    node->op = OP_STORE;
    node->varName = nameToken.value;
    node->left = parseComparison();
    node->isConstDef = true; 
    return node;
}
    if (t.type == TOKEN_BREAK) {
    consume();
    ASTNode* node = new ASTNode();
    node->type = NODE_BREAK;
    node->lineNumber = t.line;
    return node;
}
if (t.type == TOKEN_CONTINUE) {
    consume();
    ASTNode* node = new ASTNode();
    node->type = NODE_CONTINUE;
    node->lineNumber = t.line;
    return node;
}

   if (t.type == TOKEN_WRITE) {
    consume();
    ASTNode* rootNode = new ASTNode();
    rootNode->lineNumber = t.line;
    rootNode->type = NODE_OP;
    rootNode->op   = OP_PRINT;
    
    if (peek().type == TOKEN_CHAIN_DEF) {
        Token chainToken = consume();
        ASTNode* chainNode = new ASTNode();
        chainNode->type    = NODE_CHAIN_ACCESS;
        chainNode->varName = chainToken.value;
        chainNode->lineNumber = chainToken.line;
        rootNode->left = chainNode;
        return rootNode;
    }
    
   
    if (peek().type == TOKEN_VAR && peekAt(pos + 1).type == TOKEN_LBRACKET) {
        ASTNode* current = parseComparison(); 
        rootNode->left = current;
        return rootNode;
    }
    
    ASTNode* current = parseComparison();
    rootNode->left = current;
    while (peek().type == TOKEN_COMMA) {
        consume();
        current->right = parseComparison();
        current = current->right;
    }
    return rootNode;
}
  
if (t.type == TOKEN_WRITES) {
    consume();
    ASTNode* rootNode = new ASTNode();
    rootNode->lineNumber = t.line;
    rootNode->type = NODE_WRITES; 
    rootNode->op = OP_WRITES;

    auto canStartExpr = [&]() {
        switch (peek().type) {
            case TOKEN_VAR: case TOKEN_NUMBER: case TOKEN_FLOAT:
            case TOKEN_STRING: case TOKEN_BOOL: case TOKEN_LPAREN:
            case TOKEN_MINUS: case TOKEN_NOT: case TOKEN_USER_INPUT:
            case TOKEN_CHAIN_DEF: case TOKEN_CHAIN_FIELD:
            case TOKEN_CHAIN_FUNC: case TOKEN_ALIAS_CALL:
                return true;
            default:
                return false;
        }
    };

    while (canStartExpr()) {
        ASTNode* arg = parseComparison();
        if (!arg) break;
        rootNode->arguments.push_back(arg); 
        if (peek().type == TOKEN_COMMA) consume();
    }
    if (peek().type == TOKEN_SEMICOLON) consume();
    return rootNode;
}

    if (t.type == TOKEN_CLEAR) {
        consume();
        ASTNode* node = new ASTNode();
        node->lineNumber = t.line;
        node->type = NODE_CLS;
        node->op   = OP_CLS;
        return node;
    }

    if (t.type == TOKEN_INFO) {
    consume();
    ASTNode* node = new ASTNode();
    node->lineNumber = t.line;
    Token next = peek();
    
    
    if (next.type == TOKEN_WRITE || next.type == TOKEN_REPEAT || next.type == TOKEN_IF) {
        node->type    = NODE_INFO_CMD;
        node->op      = OP_INFO_CMD;
        node->varName = consume().value;
    }
    else if (next.type == TOKEN_VAR) {
       
        auto& handles = PowerManager::getInstance().handles;
        std::string name = next.value;
        
        if (handles.count(name)) {
            // loaded power module — INFO_CMD
            node->type    = NODE_INFO_CMD;
            node->op      = OP_INFO_CMD;
            node->varName = consume().value;
        } else {
            // regular variable — INFO
            node->type    = NODE_INFO;
            node->op      = OP_INFO;
            node->varName = consume().value;
        }
    }
    else if (next.type == TOKEN_CHAIN_DEF) {
        node->type    = NODE_CHAIN_INFO;
        node->op      = OP_CHAIN_INFO;
        node->varName = consume().value;
    }
    
    if (peek().type == TOKEN_SEMICOLON) consume();
    return node;
}
    if (t.type == TOKEN_CHAIN_FUNC) {
    ASTNode* node = parseFactor(); 
    return node;
}

if (t.type == TOKEN_METHOD_DEF) {
    consume(); 
    ASTNode* node = new ASTNode();
    node->type = NODE_METHOD_DEF;
    node->varName = t.value; 
    node->lineNumber = t.line;
    consume(TOKEN_LPAREN, "Expected '('");
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        Token param = consume(TOKEN_VAR, "Expected param");
        node->params.push_back(param.value);
        if (peek().type == TOKEN_COMMA) consume();
    }
    consume(TOKEN_RPAREN, "Expected ')'");
    
    ASTNode* bodyHead = nullptr;
    while (peek().type != TOKEN_SEMICOLON && peek().type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement();
        if (!stmt) continue;
        if (!bodyHead) bodyHead = stmt;
        else {
            ASTNode* cur = bodyHead;
            while (cur->right) cur = cur->right;
            cur->right = stmt;
        }
    }
    if (peek().type == TOKEN_FUNC_END) {
    AstraError::syntax(ErrCode::INVALID_SYNTAX, peek().line,
        " Chain methods end with ';' not '#ef' (only regular #f functions use '#ef')");
    consume(); 
} else {
    consume(TOKEN_SEMICOLON, "Expected ';' to end method definition");
}
node->body = bodyHead;
return node;
}

    if (t.type == TOKEN_CHAIN_DEF) {
        
    if (peekAt(pos + 1).type == TOKEN_LBRACKET) {
        std::string chainName = consume().value; 
        consume(); 
        ASTNode* indexExpr = parseExpression();
        consume(); 
        
    if (peek().type == TOKEN_COLON) {
        consume(); 
        std::string fieldName = consume().value; 
        consume(); 
        ASTNode* valueExpr = parseComparison();
        
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_DYNAMIC_FIELD_STORE; 
        node->varName = chainName;
        node->lineNumber = t.line;
        node->left      = indexExpr;   
        node->condition = valueExpr;   
        node->params.push_back(fieldName); 
        return node;
    }

        consume(); 
        ASTNode* valueExpr = parseComparison();
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_DYNAMIC_STORE;
        node->varName = chainName;
        node->lineNumber = t.line;
        node->left    = indexExpr;
        node->condition = valueExpr; 
        return node;
    }
        Token chainToken = consume();
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_DEF;
        node->varName = chainToken.value;
        node->lineNumber = chainToken.line;
        if (peek().type == TOKEN_LPAREN) {
            consume();
            while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                Token field = consume();

                if (peek().type == TOKEN_LPAREN) {
                    
                    consume(); 
                    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                        Token subField = consume();
                        node->params.push_back(field.value + ":" + subField.value);
                        if (peek().type == TOKEN_COMMA) consume();
                    }
                    consume(); 
                } else {
                    node->params.push_back(field.value);
                }

                if (peek().type == TOKEN_COMMA) consume();
            }
            consume();
        }
        
if (peek().type != TOKEN_ASSIGN) {
    return node; 
}
        consume(TOKEN_ASSIGN, "Expected '=' after chain name");
while (peek().type != TOKEN_EOF && peek().type != TOKEN_SEMICOLON) {
    if (peek().type == TOKEN_LPAREN) {
        consume();
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            if (peek().type == TOKEN_LPAREN) {
                
                consume(); 
                while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                    node->arguments.push_back(parseExpression());
                    if (peek().type == TOKEN_COMMA) consume();
                }
                consume(); 
            } else {
                node->arguments.push_back(parseExpression());
            }
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume();
    } else {
        node->arguments.push_back(parseExpression());
    }
    if (peek().type == TOKEN_COMMA) consume();
    else break;
}
return node;
    }

    if (t.type == TOKEN_CHAIN_FIELD && peekAt(pos + 1).type == TOKEN_ASSIGN) {
        Token fieldToken = consume();
        consume();
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_FIELD_STORE;
        node->varName = fieldToken.value;
        node->lineNumber = fieldToken.line;
        node->left    = parseComparison();
        return node;
    }

if (t.type == TOKEN_CHAIN_FIELD && peekAt(pos + 1).type == TOKEN_LPAREN) {
    Token fieldToken = consume(); 
    consume(); 
    ASTNode* node = new ASTNode();
    node->type = NODE_METHOD_CALL;
    node->varName = fieldToken.value; 
    node->lineNumber = fieldToken.line;
    while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
        node->arguments.push_back(parseExpression());
        if (peek().type == TOKEN_COMMA) consume();
    }
    consume(); 
    return node;
}

if (t.type == TOKEN_ALIAS_CALL) {
    
    if (peekAt(pos + 1).type == TOKEN_LBRACKET) {
        std::string chainName = consume().value; 
        consume(); 
        ASTNode* indexExpr = parseExpression();
        consume(); 

        
        if (peek().type == TOKEN_COLON) {
            consume();
            std::string fieldName = consume().value;
            consume(); 
            ASTNode* valueExpr = parseComparison();
            ASTNode* node = new ASTNode();
            node->type    = NODE_CHAIN_DYNAMIC_FIELD_STORE;
            node->varName = chainName;
            node->lineNumber = t.line;
            node->left      = indexExpr;
            node->condition = valueExpr;
            node->params.push_back(fieldName);
            return node;
        }

        
        consume(); 
        ASTNode* valueExpr = parseComparison();
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_DYNAMIC_STORE;
        node->varName = chainName;
        node->lineNumber = t.line;
        node->left    = indexExpr;
        node->condition = valueExpr;
        return node;
    }
    
    else if (peekAt(pos + 1).type == TOKEN_LPAREN) {
        std::string funcName = consume().value;
        consume();
        ASTNode* node = new ASTNode();
        node->type = NODE_FUNC_CALL;
        node->varName = funcName;
        node->lineNumber = t.line;
        while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
            node->arguments.push_back(parseExpression());
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume(); 
        return node;
    }
   
    else if (peekAt(pos + 1).type == TOKEN_ASSIGN) {
        ASTNode* node = new ASTNode();
        node->type    = NODE_OP;
        node->op      = OP_STORE;
        node->varName = consume().value;
        consume(); 
        node->left = parseComparison();
        return node;
    }
}
if (t.type == TOKEN_VAR && peekAt(pos + 1).type == TOKEN_LINK_ARROW) {
        std::string sourceName = consume().value;
        consume(); 
        
        ASTNode* node = new ASTNode();
        node->type = NODE_LINK;
        node->varName = sourceName;
        node->lineNumber = t.line;
        
        node->params.push_back(consume(TOKEN_VAR, "Expected target variable name after '->'").value);
        while (peek().type == TOKEN_COMMA) {
            consume();
            node->params.push_back(consume(TOKEN_VAR, "Expected target variable name").value);
        }
        return node;
    }

    if (t.type == TOKEN_VAR) {

    if (peekAt(pos + 1).type == TOKEN_COMMA ||
        (peekAt(pos + 1).type == TOKEN_DECREMENT && peekAt(pos + 2).type == TOKEN_VAR)) {
        return parseMultiAssignment();
    }
    
    
    if (peekAt(pos + 1).type == TOKEN_INCREMENT) {
        std::string varName = consume().value;
        consume(); 
        ASTNode* node = new ASTNode();
        node->type    = NODE_OP;
        node->op      = OP_STORE;
        node->varName = varName;
        node->lineNumber = t.line;
        ASTNode* addNode = new ASTNode();
        addNode->type      = NODE_OP;
        addNode->op        = OP_ADD;
        addNode->lineNumber = t.line;
        ASTNode* varNode = new ASTNode();
        varNode->type    = NODE_VAR;
        varNode->varName = varName;
        varNode->lineNumber = t.line;
        ASTNode* oneNode = new ASTNode();
        oneNode->type           = NODE_LITERAL;
        oneNode->value.type     = VAL_INT;
        oneNode->value.num      = 1;
        addNode->left  = varNode;
        addNode->right = oneNode;
        node->left = addNode;
        return node;
    }
    
if (peekAt(pos + 1).type == TOKEN_DECREMENT) {
    std::string varName = consume().value;
    consume(); 
    ASTNode* node = new ASTNode();
    node->type    = NODE_OP;
    node->op      = OP_STORE;
    node->varName = varName;
    node->lineNumber = t.line;
    ASTNode* subNode = new ASTNode();
    subNode->type       = NODE_OP;
    subNode->op         = OP_SUB;
    subNode->lineNumber = t.line;
    ASTNode* varNode = new ASTNode();
    varNode->type    = NODE_VAR;
    varNode->varName = varName;
    varNode->lineNumber = t.line;
    ASTNode* oneNode = new ASTNode();
    oneNode->type       = NODE_LITERAL;
    oneNode->value.type = VAL_INT;
    oneNode->value.num  = 1;
    subNode->left  = varNode;
    subNode->right = oneNode;
    node->left = subNode;
    return node;
}
    
    if (peekAt(pos + 1).type == TOKEN_PLUS_ASSIGN) {
        std::string varName = consume().value;
        consume(); 
        ASTNode* node = new ASTNode();
        node->type    = NODE_OP;
        node->op      = OP_STORE;
        node->varName = varName;
        node->lineNumber = t.line;
        ASTNode* addNode = new ASTNode();
        addNode->type       = NODE_OP;
        addNode->op         = OP_ADD;
        addNode->lineNumber = t.line;
        ASTNode* varNode = new ASTNode();
        varNode->type    = NODE_VAR;
        varNode->varName = varName;
        varNode->lineNumber = t.line;
        addNode->left  = varNode;
        addNode->right = parseComparison();
        node->left = addNode;
        return node;
    }
    
    if (peekAt(pos + 1).type == TOKEN_MINUS_ASSIGN) {
        std::string varName = consume().value;
        consume(); 
        ASTNode* node = new ASTNode();
        node->type    = NODE_OP;
        node->op      = OP_STORE;
        node->varName = varName;
        node->lineNumber = t.line;
        ASTNode* subNode = new ASTNode();
        subNode->type       = NODE_OP;
        subNode->op         = OP_SUB;
        subNode->lineNumber = t.line;
        ASTNode* varNode = new ASTNode();
        varNode->type    = NODE_VAR;
        varNode->varName = varName;
        varNode->lineNumber = t.line;
        subNode->left  = varNode;
        subNode->right = parseComparison();
        node->left = subNode;
        return node;
    }

if (peekAt(pos + 1).type == TOKEN_LBRACKET) {
    std::string chainName = consume().value; 
    consume(); 
    ASTNode* indexExpr = parseExpression();
    consume(); 
    
   
    if (peek().type == TOKEN_COLON) {
        consume(); 
        std::string fieldOrMethod = consume().value; 
        
        
        if (peek().type == TOKEN_LPAREN) {
            consume(); 
            ASTNode* node = new ASTNode();
            node->type    = NODE_CHAIN_DYNAMIC_METHOD_CALL;
            node->varName = chainName;
            node->lineNumber = t.line;
            node->left    = indexExpr;
            node->params.push_back(fieldOrMethod);
            while (peek().type != TOKEN_RPAREN && peek().type != TOKEN_EOF) {
                node->arguments.push_back(parseExpression());
                if (peek().type == TOKEN_COMMA) consume();
            }
            consume(); 
            return node;
        }
        
        
        consume(); 
        ASTNode* valueExpr = parseComparison();
        ASTNode* node = new ASTNode();
        node->type    = NODE_CHAIN_DYNAMIC_FIELD_STORE;
        node->varName = chainName;
        node->lineNumber = t.line;
        node->left      = indexExpr;
        node->condition = valueExpr;
        node->params.push_back(fieldOrMethod);
        return node;
    }
    
    
    consume(); 
    ASTNode* valueExpr = parseComparison();
    ASTNode* node = new ASTNode();
    node->type    = NODE_CHAIN_DYNAMIC_STORE;
    node->varName = chainName;
    node->lineNumber = t.line;
    node->left    = indexExpr;
    node->condition = valueExpr;
    return node;
}
    
if (peekAt(pos + 1).type == TOKEN_ASSIGN) {
    ASTNode* node = new ASTNode();
    node->type    = NODE_OP;
    node->op      = OP_STORE;
    node->varName = consume().value;
    consume(); 
    node->left = (peek().type == TOKEN_USER_INPUT) ? parseFactor() : parseComparison();
    
    
    if (peek().type == TOKEN_ALIAS) {
        consume(); 
        Token aliasTok = consume(TOKEN_VAR, "Expected alias name after 'alias'");
        node->params.push_back(aliasTok.value);
        
    }
    
    return node;
}
    
    else if (peekAt(pos + 1).type == TOKEN_VAR) {
        std::string alias  = consume().value;
        std::string module = consume().value;
        PowerManager::getInstance().load(module);
        return nullptr;
    }
    
    else if (peekAt(pos + 1).type == TOKEN_LPAREN) {
    Token funcTok = consume();              
    std::string funcName = funcTok.value;
    consume(); 
    ASTNode* funcNode = parseFunctionCall(funcName);
    if (funcNode) funcNode->lineNumber = funcTok.line;   

    if (peek().type == TOKEN_LBRACKET) {
        consume(); 
        ASTNode* modNode = new ASTNode();
        modNode->type = NODE_MODIFIER_CALL;
        modNode->varName = funcName;
        modNode->lineNumber = funcTok.line;   
        modNode->left = funcNode; 
        
        while (peek().type != TOKEN_RBRACKET && peek().type != TOKEN_EOF) {
            Token modName = consume(TOKEN_VAR, "Expected modifier name");
            modNode->params.push_back(modName.value);
            if (peek().type == TOKEN_COMMA) consume();
        }
        consume(); 
        return modNode;
    }
    
    static const std::set<std::string> voidPowerFuncs = {
        "close", "drop", "open", "query", "dbfetch", "bulkinsert"
    };

    if (funcNode && funcNode->type == NODE_POWER_CALL && 
        !voidPowerFuncs.count(funcNode->varName)) {
        ASTNode* printNode = new ASTNode();
        printNode->type = NODE_OP;
        printNode->op   = OP_PRINT;
        printNode->lineNumber = funcTok.line;   
        printNode->left = funcNode;
        return printNode;
    }
    return funcNode;
}
    
    else {
    ASTNode* node = new ASTNode();
    node->type = NODE_STANDALONE_VAR;
    Token tok = consume();
    node->varName = tok.value;
    node->lineNumber = tok.line; 
    return node;
}
}

    return parseComparison();
}

ASTNode* Parser::parseIfStatement(bool isChained) {
    Token t = consume();
    ASTNode* node = new ASTNode();
    node->lineNumber = t.line;
    node->type = NODE_IF;
    int startPos = pos; 
    node->condition = parseComparison();
    int endPos = pos;   

    bool isValidCondition = node->condition &&
    (
        (node->condition->type == NODE_OP &&
         (node->condition->op == OP_GT  || node->condition->op == OP_LT  ||
          node->condition->op == OP_EQ  || node->condition->op == OP_NEQ ||
          node->condition->op == OP_GTE || node->condition->op == OP_LTE ||
          node->condition->op == OP_AND || node->condition->op == OP_OR) &&
         node->condition->right != nullptr)
        ||
        (node->condition->type == NODE_NOT)
        ||
        (node->condition->type == NODE_LITERAL)
        ||
        (node->condition->type == NODE_VAR)
        ||
        (node->condition->type == NODE_POWER_CALL)
        ||
        (node->condition->type == NODE_FUNC_CALL)
    );

    if (isValidCondition) {
        TokenType nextType = peek().type;
        static const std::set<TokenType> invalidAfterCondition = {
            TOKEN_COMMA, TOKEN_ASSIGN,
            TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD, TOKEN_POW,
            TOKEN_RPAREN, TOKEN_RBRACKET, TOKEN_COLON, TOKEN_DOT,
            TOKEN_EQ, TOKEN_NEQ, TOKEN_LT, TOKEN_GT, TOKEN_LTE, TOKEN_GTE,
            TOKEN_AND, TOKEN_OR,
            TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_LSHIFT, TOKEN_RSHIFT
        };
        if (invalidAfterCondition.count(nextType)) {
            isValidCondition = false;
        }
    }

    if (!isValidCondition) {
    std::string srcText = "if ";
    for (int i = startPos; i < endPos && i < (int)tokens.size(); i++) {
        
        TokenType tt = tokens[i].type;
        if (tt == TOKEN_WRITE || tt == TOKEN_IF || 
            tt == TOKEN_REPEAT || tt == TOKEN_EOF) break;
        srcText += tokens[i].value + " ";
    }
    AstraError::syntax(ErrCode::INVALID_SYNTAX, t.line, " after '" + srcText + "'");
    skipBlock();
    return makeErrorNode(t.line);
}

    
    parseBlock(node->thenBranch);
    if (peek().type == TOKEN_ELSE) {
    Token elseTok = consume();
    if (peek().type == TOKEN_IF && peek().line == elseTok.line) {
        
        node->elseBranch = parseIfStatement(true);
    }
    else if (peek().type == TOKEN_IF) {
        
        node->elseBranch = parseIfStatement(false);
    }
    else {
        parseBlock(node->elseBranch);
    }
}
    
    if (!isChained) {
    if (peek().type == TOKEN_SEMICOLON) {
        consume();
    } else if (peek().type != TOKEN_EOF) {
        AstraError::syntax(ErrCode::MISSING_SEMICOLON, t.line, "");
        return makeErrorNode(t.line);
    }
    }
    return node;
}