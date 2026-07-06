/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "common.h"

std::string valueToString(const Value& v) {
    if (v.type == VAL_STR) return v.str;
    if (v.type == VAL_PTR) return v.str;
    if (v.type == VAL_FLOAT) {
        std::string s = std::to_string(v.decimal);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }
    if (v.type == VAL_BOOL) {
        return v.tristate == AST_TRUE ? "TRUE" : (v.tristate == AST_FALSE ? "FALSE" : "MAYBE");
    }
    if (v.type == VAL_FILE) {
        return "<file:" + std::to_string(v.num) + ">";
    }
    return std::to_string(v.num); 
}

Value stringToValue(const std::string& s) {
    Value v;
    v.isInitialized = true;

    if (s == "TRUE" || s == "true") {
        v.type = VAL_BOOL;
        v.tristate = AST_TRUE;
    }
    else if (s == "FALSE" || s == "false") {
        v.type = VAL_BOOL;
        v.tristate = AST_FALSE;
    }
    else if (s == "MAYBE") {
        v.type = VAL_BOOL;
        v.tristate = AST_MAYBE;
    }
    else if (!s.empty() && (isdigit((unsigned char)s[0]) || (s[0] == '-' && s.size() > 1 && isdigit((unsigned char)s[1])))) {
        
        bool isNumeric = true;
        bool hasDot = false;
        for (size_t i = 1; i < s.size(); i++) {
            if (s[i] == '.') {
                if (hasDot) { isNumeric = false; break; }
                hasDot = true;
            } else if (!isdigit((unsigned char)s[i])) {
                isNumeric = false;
                break;
            }
        }
        if (isNumeric) {
            if (hasDot) {
                v.type = VAL_FLOAT;
                v.decimal = std::stod(s);
            } else {
                v.type = VAL_INT;
                v.num = std::stoll(s);
            }
        } else {
            v.type = VAL_STR;
            v.str = s;
        }
    }
    else {
        v.type = VAL_STR;
        v.str = s;
    }

    return v;
}