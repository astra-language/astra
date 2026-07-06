/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */


#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstdint> 
#include <vector>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define WHITE   "\033[1;37m"
#define MAGENTA "\033[35m"
#define BOLD    "\033[1m"
#define ORANGE  "\033[38;5;208m"

enum OpCode : int {
    // Load & Store
    OP_LOAD, OP_LOAD_STR, OP_LOAD_FLOAT, OP_LOAD_BOOL,
    OP_LOAD_LONG,
    OP_LOAD_VAR,
    OP_STORE,

    // Arithmetic
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_NEG, OP_POW,

    // Bitwise
    OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR, OP_LSHIFT, OP_RSHIFT,

    // Comparison
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE,

    // Logical
    OP_AND, OP_OR, OP_NOT,

    // Control Flow
    OP_JMP, OP_JZ, 

    // Loop Control
    OP_LOOP_START, 
    OP_LOOP_COND,  
    OP_LOOP_INC,   
    OP_LOOP_STEP,
    

    //info
    OP_INFO,     
    OP_INFO_CMD,

    // System
    OP_PRINT, OP_HALT,
    OP_USER_INPUT,OP_CLS,

    OP_FUNC_DEF,
    OP_FUNC_CALL,
    OP_RETURN_VAL,
    OP_RETURN,

    OP_BREAK,
    OP_CONTINUE,
    OP_HINT,

    OP_CHAIN_LEN,
    OP_CHAIN_SORT,
    OP_CHAIN_MERGE,
    OP_CHAIN_UNIQUE,
    OP_CHAIN_SUM,
    OP_CHAIN_AVG,
    OP_CHAIN_MAX,
    OP_CHAIN_MIN,
    OP_CHAIN_CONTAINS,
    OP_CHAIN_INDEXOF,
    OP_CHAIN_REVERSE,
    OP_CHAIN_JOIN,

    //power
    OP_POWER_CALL,

    OP_CHAIN_STORE,   
    OP_CHAIN_LOAD,    
    OP_CHAIN_PRINT,    
    OP_CHAIN_FIELD_STORE,
    OP_CHAIN_FIELD_LOAD,
    OP_CHAIN_DYNAMIC_LOAD,  
    OP_CHAIN_DYNAMIC_STORE, 
    OP_CHAIN_DYNAMIC_FIELD_STORE,
    OP_CHAIN_DYNAMIC_FIELD_LOAD,
    OP_CHAIN_SELF,
    OP_CHAIN_INFO,
    OP_METHOD_DEF,
    OP_METHOD_CALL,
    OP_CHAIN_DYNAMIC_METHOD_CALL,
    OP_CHAIN_PRINT_FROM_STACK,

    OP_CALL_ADR,
    OP_CALL_VAL,
    

    // File operations
    OP_FILE_CREATE,
    OP_FILE_READ,
    OP_FILE_PLUS,
    OP_FILE_CLEAR,
    OP_FILE_REMOVE,
    OP_FILE_CLOSE,
    OP_FILE_EOF,
    OP_FILE_FETCH,

    OP_CHECK_INT,
    OP_CHECK_STR,
    OP_CHECK_RANGE,
    OP_CHECK_DEFAULT,
    OP_CHECK_END,

    OP_WRITES,
    OP_JSON_PARSE,
    OP_TO_JSON,

    OP_EXE_CHAIN,

    OP_WHEN_START,
    OP_WHEN_END,
    OP_THEN_CHECK,
    OP_THEN_END,

    OP_DEALIAS,
    OP_ALIAS,

    OP_MODIFIER_CALL,

     OP_LINK,
     OP_JSON_PRETTY,

};

enum TriState { AST_FALSE, AST_TRUE, AST_MAYBE };
enum ValueType { VAL_INT, VAL_STR, VAL_FLOAT, VAL_BOOL, VAL_PTR, VAL_FILE };

struct Value {
    ValueType type;
    bool isInitialized; 
    long long num;
    double decimal;
    TriState tristate; 
    std::string str; 
    bool isPoisoned = false; 
    bool isConst = false;

    Value() : type(VAL_INT), isInitialized(false), num(0) {}
};
// AST Node
enum NodeType { NODE_OP, NODE_LITERAL, NODE_VAR, NODE_IF, NODE_USER_INPUT , NODE_REPEAT, NODE_REPEAT_COND, NODE_CLS, NODE_INFO, NODE_INFO_CMD,  NODE_ERROR, NODE_POWER_CALL, NODE_NEGATE,NODE_FUNC_DEF,
NODE_FUNC_CALL, NODE_RETURN, NODE_CHAIN_DEF, NODE_CHAIN_ACCESS, NODE_CHAIN_FIELD_ACCESS,NODE_CHAIN_FIELD_STORE, NODE_CHAIN_DYNAMIC, NODE_CHAIN_DYNAMIC_STORE, NODE_NOT, NODE_BREAK, NODE_CONTINUE, NODE_CHAIN_FUNC, NODE_CHAIN_DYNAMIC_FIELD_STORE,NODE_CHAIN_DYNAMIC_FIELD_LOAD,
 NODE_CHAIN_INFO,NODE_METHOD_DEF,NODE_METHOD_CALL,NODE_CHECK,NODE_CHAIN_DYNAMIC_METHOD_CALL,NODE_STANDALONE_VAR,NODE_WRITES, NODE_WHEN, NODE_THEN, NODE_ATTACH,NODE_DEALIAS,NODE_ALIAS,
 NODE_MODIFIER_DEF,
 NODE_MODIFIER_CALL,NODE_EXE, NODE_LINK,NODE_INCLUDE
  
};


enum TokenType {
    TOKEN_VAR, TOKEN_NUMBER, TOKEN_FLOAT, TOKEN_BOOL, TOKEN_STRING, 
    TOKEN_ASSIGN,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD,
    TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_LSHIFT, TOKEN_RSHIFT,
    TOKEN_EQ, TOKEN_NEQ, TOKEN_LT, TOKEN_GT, TOKEN_LTE, TOKEN_GTE,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_POW,
    TOKEN_WRITE, TOKEN_IF, TOKEN_ELSE, TOKEN_SEMICOLON,TOKEN_CLEAR, 
    TOKEN_WRITES,TOKEN_NEWLINE,
    TOKEN_USER_INPUT,        
    TOKEN_PROMPT_TEXT,  
    TOKEN_REPEAT, TOKEN_TO, TOKEN_COMMA,
    TOKEN_FUNC_START,  
    TOKEN_FUNC_END,
    TOKEN_RETURN,
    TOKEN_EOF, TOKEN_ERROR, TOKEN_INFO,
    TOKEN_CHAIN_DEF,    
    TOKEN_CHAIN_FIELD,  
    TOKEN_COLON,     
    TOKEN_CHECK,
    TOKEN_WHEN,
    TOKEN_THEN,
    TOKEN_END,TOKEN_CONST,
    TOKEN_LBRACKET,TOKEN_METHOD_DEF,
    TOKEN_RBRACKET,TOKEN_BREAK, TOKEN_CONTINUE,TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN, TOKEN_INCREMENT, TOKEN_DECREMENT, TOKEN_CHAIN_FUNC,
    TOKEN_ATTACH,
    TOKEN_AS,
    TOKEN_INCLUDE,
    TOKEN_ONLY,
    TOKEN_EXCEPT,
    TOKEN_DOT,        
    TOKEN_ALIAS_CALL, 
    TOKEN_ALIAS,
    TOKEN_DEALIAS,
    TOKEN_MODIFIER_START,   
    TOKEN_MOD_BEFORE,       
    TOKEN_MOD_AFTER, 
    TOKEN_EXE,
    TOKEN_LINK_ARROW       
};

struct ASTNode {
    NodeType type;
    OpCode op;
    Value value;
    std::string varName;
    int lineNumber;

    bool isConstDef = false;

    bool isChainExe = false;
    
    ASTNode *left;
    ASTNode *right;
    ASTNode* next = nullptr;   

    
    ASTNode *condition;
    ASTNode *thenBranch;
    ASTNode *elseBranch;

    std::vector<ASTNode*> arguments;

    std::vector<ASTNode*> writesNodes;

    
    std::string iteratorName; 
    ASTNode *startExpr;       
    ASTNode *endExpr;         
    ASTNode *body;            
    ASTNode* stepExpr = nullptr;
    

    std::vector<std::string> params;

    ASTNode() : type(NODE_LITERAL), op(OP_ADD), left(nullptr), right(nullptr), 
                condition(nullptr), thenBranch(nullptr), elseBranch(nullptr), 
                startExpr(nullptr), endExpr(nullptr), body(nullptr), varName("") {
        value.type = VAL_INT;
        value.num = 0; 
    }
};
struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<ASTNode*> body;

    FunctionDef() : body() {}
};

const int MAX_MEMORY = 20000;

std::string valueToString(const Value& v);
Value stringToValue(const std::string& s);   

#endif