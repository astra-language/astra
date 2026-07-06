/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "lexer.h"
#include <cctype>
#include <algorithm>
#include <iostream>

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < source.length()) {
        char current = source[pos];

        if (current == '\n') { line++; pos++; continue; }
        if (isspace(current)) { pos++; continue; }

        // Strings
        if (current == '"') {
            std::string val; pos++;
            while (pos < source.length() && source[pos] != '"') {
                if (source[pos] == '\\' && pos + 1 < source.length() && source[pos+1] == '"') {
                    val += '"';      
                    pos += 2;
                } else {
                    val += source[pos++];
                }
            }
            pos++;
            tokens.push_back({TOKEN_STRING, val, 0, 0.0, AST_FALSE, line});
        }

        // user: input
        else if (source.substr(pos, 5) == "user:") {
            tokens.push_back({TOKEN_USER_INPUT, "user:", 0, 0.0, AST_FALSE, line});
            pos += 5;
            std::string promptText = "";
            while (pos < source.length() && source[pos] != '\n' && source[pos] != '\r') {
                promptText += source[pos++];
            }
            tokens.push_back({TOKEN_PROMPT_TEXT, promptText, 0, 0.0, AST_FALSE, line});
        }

        // Variables & Keywords
        else if (isalpha(current) || current == '_') {
            std::string val;
            while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
                val += source[pos++];
            }

            std::string lowerVal = val;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::tolower);

    if (pos < source.length() && source[pos] == '.') {
    pos++; 
    std::string method;
    while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
        method += source[pos++];
    }
    std::string fullName = val + "." + method; 


    if (pos < source.length() && source[pos] == ':') {
        pos++; 
        std::string chainSuffix;
        while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
            chainSuffix += source[pos++];
        }
        if (chainSuffix == "n") {
            if (pos < source.length() && source[pos] == ':') {
                pos++;
                std::string methodName;
                while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
                    methodName += source[pos++];
                }
                tokens.push_back({TOKEN_METHOD_DEF, fullName + ":n:" + methodName, 0, 0.0, AST_FALSE, line});
            } else {
                
                tokens.push_back({TOKEN_CHAIN_DEF, fullName, 0, 0.0, AST_FALSE, line});
            }
        } else {
           
            tokens.push_back({TOKEN_CHAIN_FIELD, fullName + ":" + chainSuffix, 0, 0.0, AST_FALSE, line});
        }
    } else {
        
        tokens.push_back({TOKEN_ALIAS_CALL, fullName, 0, 0.0, AST_FALSE, line});
    }
    continue;
}
            
            if (pos < source.length() && source[pos] == ':') {
    
    if (lowerVal == "end") {
        tokens.push_back({TOKEN_END, val, 0, 0.0, AST_FALSE, line});
        pos++; 
    }
     
    else if (lowerVal == "when") {
        tokens.push_back({TOKEN_WHEN, val, 0, 0.0, AST_FALSE, line});
    }
    else if (lowerVal == "then") {
    tokens.push_back({TOKEN_THEN, val, 0, 0.0, AST_FALSE, line});
  
}
else if (lowerVal == "before") {
    if (pos < source.length() && source[pos] == ':') {
        pos++;
        tokens.push_back({TOKEN_MOD_BEFORE, "before:", 0, 0.0, AST_FALSE, line});
    } else {
        tokens.push_back({TOKEN_VAR, val, 0, 0.0, AST_FALSE, line});
    }
}
else if (lowerVal == "after") {
    if (pos < source.length() && source[pos] == ':') {
        pos++;
        tokens.push_back({TOKEN_MOD_AFTER, "after:", 0, 0.0, AST_FALSE, line});
    } else {
        tokens.push_back({TOKEN_VAR, val, 0, 0.0, AST_FALSE, line});
    }
}
    else {
        pos++;
        if (pos < source.length() && (isalpha(source[pos]) || source[pos] == '_')) {
            std::string suffix;
            while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
                suffix += source[pos++];
            }
            if (suffix == "n") {
                if (pos < source.length() && source[pos] == ':') {
                    pos++; 
                    std::string methodName;
                    while (pos < source.length() && (isalnum(source[pos]) || source[pos] == '_')) {
                        methodName += source[pos++];
                    }
                    tokens.push_back({TOKEN_METHOD_DEF, val + ":n:" + methodName, 0, 0.0, AST_FALSE, line});
                } else {
                    tokens.push_back({TOKEN_CHAIN_DEF, val, 0, 0.0, AST_FALSE, line});
                }
            } else {
                tokens.push_back({TOKEN_CHAIN_FIELD, val + ":" + suffix, 0, 0.0, AST_FALSE, line});
            }
        } else {
            tokens.push_back({TOKEN_VAR, val, 0, 0.0, AST_FALSE, line});
            tokens.push_back({TOKEN_COLON, ":", 0, 0.0, AST_FALSE, line}); 
        }
    }
}           // keywords
            else if (lowerVal == "when") tokens.push_back({TOKEN_WHEN, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "then") tokens.push_back({TOKEN_THEN, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "write")  tokens.push_back({TOKEN_WRITE,  val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "writes") tokens.push_back({TOKEN_WRITES, val, 0, 0.0, AST_FALSE, line}); 
            else if (lowerVal == "if")     tokens.push_back({TOKEN_IF,     val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "else")   tokens.push_back({TOKEN_ELSE,   val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "true")   tokens.push_back({TOKEN_BOOL,   val, 0, 0.0, AST_TRUE,  line});
            else if (lowerVal == "false")  tokens.push_back({TOKEN_BOOL,   val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "maybe")  tokens.push_back({TOKEN_BOOL,   val, 0, 0.0, AST_MAYBE, line});
            else if (lowerVal == "repeat") tokens.push_back({TOKEN_REPEAT, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "to")     tokens.push_back({TOKEN_TO,     val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "attach") tokens.push_back({TOKEN_ATTACH, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "as")     tokens.push_back({TOKEN_AS,     val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "include") tokens.push_back({TOKEN_INCLUDE, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "only")    tokens.push_back({TOKEN_ONLY,    val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "except")  tokens.push_back({TOKEN_EXCEPT,  val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "const") tokens.push_back({TOKEN_CONST, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "clear") {
                 if (pos < source.length() && source[pos] == '(') {
                        tokens.push_back({TOKEN_VAR, val, 0, 0.0, AST_FALSE, line});
                    } else {
                        tokens.push_back({TOKEN_CLEAR, val, 0, 0.0, AST_FALSE, line});
                    }
}
            else if (lowerVal == "cls")    tokens.push_back({TOKEN_CLEAR,  val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "info")   tokens.push_back({TOKEN_INFO,   val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "return") tokens.push_back({TOKEN_RETURN, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "break")    tokens.push_back({TOKEN_BREAK,    val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "continue") tokens.push_back({TOKEN_CONTINUE, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "len")   tokens.push_back({TOKEN_CHAIN_FUNC, "len",   0, 0.0, AST_FALSE, line});
            else if (lowerVal == "sort")  tokens.push_back({TOKEN_CHAIN_FUNC, "sort",  0, 0.0, AST_FALSE, line});
            else if (lowerVal == "merge") tokens.push_back({TOKEN_CHAIN_FUNC, "merge", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "unique") tokens.push_back({TOKEN_CHAIN_FUNC, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "self") tokens.push_back({TOKEN_CHAIN_FUNC, "self", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "sum")     tokens.push_back({TOKEN_CHAIN_FUNC, "sum",     0, 0.0, AST_FALSE, line});
            else if (lowerVal == "avg")     tokens.push_back({TOKEN_CHAIN_FUNC, "avg",     0, 0.0, AST_FALSE, line});
            else if (lowerVal == "cmax") tokens.push_back({TOKEN_CHAIN_FUNC, "chainMax", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "cmin") tokens.push_back({TOKEN_CHAIN_FUNC, "chainMin", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "reverse") tokens.push_back({TOKEN_CHAIN_FUNC, "reverse", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "contains") tokens.push_back({TOKEN_CHAIN_FUNC, "contains", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "indexof") tokens.push_back({TOKEN_CHAIN_FUNC, "indexOf", 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "join")    tokens.push_back({TOKEN_CHAIN_FUNC, "join",    0, 0.0, AST_FALSE, line});
            else if (lowerVal == "check") tokens.push_back({TOKEN_CHECK, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "end")   tokens.push_back({TOKEN_END,   val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "alias") tokens.push_back({TOKEN_ALIAS, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "dealias") tokens.push_back({TOKEN_DEALIAS, val, 0, 0.0, AST_FALSE, line});
            else if (lowerVal == "exe") tokens.push_back({TOKEN_EXE, val, 0, 0.0, AST_FALSE, line});
            else tokens.push_back({TOKEN_VAR, val, 0, 0.0, AST_FALSE, line});
        }

        // Numbers
        else if (isdigit(current)) {
            std::string valStr;
            bool isFloat = false;
            while (pos < source.length() && (isdigit(source[pos]) || source[pos] == '.')) {
                if (source[pos] == '.') isFloat = true;
                valStr += source[pos++];
            }
            if (isFloat)
                tokens.push_back({TOKEN_FLOAT,  valStr, 0, std::stod(valStr), AST_FALSE, line});
            else
                tokens.push_back({TOKEN_NUMBER, valStr, std::stoll(valStr), 0.0, AST_FALSE, line});
        }

        // Operators
        else if (current == '=') {
            if (pos + 1 < source.length() && source[pos + 1] == '=') {
                tokens.push_back({TOKEN_EQ, "==", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                tokens.push_back({TOKEN_ASSIGN, "=", 0, 0.0, AST_FALSE, line}); pos++;
            }
        }
        else if (current == '&') {
            if (pos + 1 < source.length() && source[pos + 1] == '&') {
                tokens.push_back({TOKEN_AND, "&&", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                pos++;   
            }
        }
        else if (current == '|') {
            if (pos + 1 < source.length() && source[pos + 1] == '|') {
                tokens.push_back({TOKEN_OR, "||", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                pos++;   
            }
        }
        else if (current == '!') {
            if (pos + 1 < source.length() && source[pos + 1] == '=') {
                tokens.push_back({TOKEN_NEQ, "!=", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                tokens.push_back({TOKEN_NOT, "!", 0, 0.0, AST_FALSE, line}); pos++;
            }
        }
        else if (current == '>') {
            if (pos + 1 < source.length() && source[pos + 1] == '=') {
                tokens.push_back({TOKEN_GTE, ">=", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                tokens.push_back({TOKEN_GT, ">", 0, 0.0, AST_FALSE, line}); pos++;
            }
        }
        else if (current == '<') {
            if (pos + 1 < source.length() && source[pos + 1] == '=') {
                tokens.push_back({TOKEN_LTE, "<=", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                tokens.push_back({TOKEN_LT, "<", 0, 0.0, AST_FALSE, line}); pos++;
            }
        }
        else if (current == '.') { 
    tokens.push_back({TOKEN_DOT, ".", 0, 0.0, AST_FALSE, line}); 
    pos++; 
}
        else if (current == '(') { tokens.push_back({TOKEN_LPAREN,   "(", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == ')') { tokens.push_back({TOKEN_RPAREN,   ")", 0, 0.0, AST_FALSE, line}); pos++; }
        
        else if (current == '[') { tokens.push_back({TOKEN_LBRACKET, "[", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == ']') { tokens.push_back({TOKEN_RBRACKET, "]", 0, 0.0, AST_FALSE, line}); pos++; }
       
        else if (current == '+') {
            if (pos + 1 < source.length() && source[pos + 1] == '=') {
                tokens.push_back({TOKEN_PLUS_ASSIGN, "+=", 0, 0.0, AST_FALSE, line});
                pos += 2;
            } else if (pos + 1 < source.length() && source[pos + 1] == '+') {
                tokens.push_back({TOKEN_INCREMENT, "++", 0, 0.0, AST_FALSE, line});
                pos += 2;
            } else {
                tokens.push_back({TOKEN_PLUS, "+", 0, 0.0, AST_FALSE, line});
                pos++;
            }
}

else if (current == '-') {
    
    if (pos + 1 < source.length() && source[pos + 1] == '=') {
        tokens.push_back({TOKEN_MINUS_ASSIGN, "-=", 0, 0.0, AST_FALSE, line});
        pos += 2;
    } 
    
    else if (pos + 1 < source.length() && source[pos + 1] == '-') {
        tokens.push_back({TOKEN_DECREMENT, "--", 0, 0.0, AST_FALSE, line});
        pos += 2;
    } 

    else if (pos + 1 < source.length() && source[pos + 1] == '>') {
        tokens.push_back({TOKEN_LINK_ARROW, "->", 0, 0.0, AST_FALSE, line});
        pos += 2;
    }
    
    else {
        tokens.push_back({TOKEN_MINUS, "-", 0, 0.0, AST_FALSE, line});
        pos++;
    }
}
        else if (current == '*') { tokens.push_back({TOKEN_STAR,     "*", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == '/') { tokens.push_back({TOKEN_SLASH,    "/", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == '%') { tokens.push_back({TOKEN_MOD,      "%", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == '^') { tokens.push_back({TOKEN_POW,      "^", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == ',') { tokens.push_back({TOKEN_COMMA,    ",", 0, 0.0, AST_FALSE, line}); pos++; }
        else if (current == ';') { tokens.push_back({TOKEN_SEMICOLON,";", 0, 0.0, AST_FALSE, line}); pos++; }
        
else if (current == ':') { 
    tokens.push_back({TOKEN_COLON, ":", 0, 0.0, AST_FALSE, line}); 
    pos++; 
}

       
else if (pos + 2 < source.length() && source.substr(pos, 3) == "'''") {
    pos += 3; 
    while (pos + 2 < source.length()) {
        if (source.substr(pos, 3) == "'''") {
            pos += 3;
            break;
        }
        if (source[pos] == '\n') line++;
        pos++;
    }
    continue;
}

else if (current == '\\') {
    if (pos + 1 < source.length() && source[pos + 1] == '\\') {
        while (pos < source.length() && source[pos] != '\n') {
            pos++;
        }
    } else {
        pos++; 
    }
    continue;
}

        else if (current == '#') {
            if (pos + 2 < source.length() && source.substr(pos, 3) == "#ef") {
                tokens.push_back({TOKEN_FUNC_END,      "#ef", 0, 0.0, AST_FALSE, line}); pos += 3;
            } else if (pos + 1 < source.length() && source.substr(pos, 2) == "#f") {
                tokens.push_back({TOKEN_FUNC_START,    "#f",  0, 0.0, AST_FALSE, line}); pos += 2;
            } else if (pos + 1 < source.length() && source.substr(pos, 2) == "#m") {
                tokens.push_back({TOKEN_MODIFIER_START, "#m", 0, 0.0, AST_FALSE, line}); pos += 2;
            } else {
                pos++;
            }
        }
        else { pos++; }
    }
    tokens.push_back({TOKEN_EOF, "", 0, 0.0, AST_FALSE, line});
    return tokens;
}
