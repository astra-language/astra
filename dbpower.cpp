/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "astra_sdk.h"
#include "vm.h"
#include "error.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
    #define ASTRA_EXPORT extern "C" __declspec(dllexport)
#else
    #define ASTRA_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static ErrorReportFn g_reportError = nullptr;

ASTRA_EXPORT void astra_set_error(ErrorReportFn fn) {
    g_reportError = fn;
}

ChainStoreFn g_chainStore = nullptr;
static sqlite3* g_db = nullptr;

// ── Helpers ──────────────────────────────────────────────────────────────────
Value mkStr(const std::string& s) {
    Value v; v.type = VAL_STR; v.str = s; v.isInitialized = true; return v;
}
Value mkInt(long long n) {
    Value v; v.type = VAL_INT; v.num = n; v.isInitialized = true; return v;
}
Value mkFloat(double d) {
    Value v; v.type = VAL_FLOAT; v.decimal = d; v.isInitialized = true; return v;
}


static std::string escapeIdentifier(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    return out;
}

static bool requireDb(AstraVM* vm) {
    if (!g_db) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            "No database open — call open(\"file.db\") first");
        return false;
    }
    return true;
}

// ── open("file.db") ──────────────────────────────────────────────────────────
void db_open(AstraVM* vm) {
    Value v = vm->pop();
    if (v.type != VAL_STR || v.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine,
            "open() expects a non-empty file path string");
        return;
    }

    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
    }

    sqlite3* newDb = nullptr;
    int rc = sqlite3_open(v.str.c_str(), &newDb);
    if (rc != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("open() failed - ") + (newDb ? sqlite3_errmsg(newDb) : "unknown error"));
        if (newDb) sqlite3_close(newDb);   
        g_db = nullptr;
        return;
    }
    g_db = newDb;
}

// ── close() ──────────────────────────────────────────────────────────────────
void db_close(AstraVM* vm) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
    }
}

// ── query("SQL") — INSERT, CREATE, UPDATE, DELETE ────────────────────────────
void db_query(AstraVM* vm) {
    Value v = vm->pop();
    if (!requireDb(vm)) return;
    if (v.type != VAL_STR || v.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "query() expects a non-empty SQL string");
        return;
    }

    char* err = nullptr;
    int rc = sqlite3_exec(g_db, v.str.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("query() failed - ") + (err ? err : "unknown error"));
        sqlite3_free(err);
    }
}

// ── fetch("SELECT ...") ───────────────────────────────────────────────────────
void db_fetch(AstraVM* vm) {
    Value sqlVal   = vm->pop();
    Value chainVal = vm->pop();

    if (!requireDb(vm)) return;
    if (sqlVal.type != VAL_STR || sqlVal.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "fetch() expects a non-empty SQL string");
        return;
    }
    if (chainVal.type != VAL_STR || chainVal.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "fetch() expects a chain name as first argument");
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sqlVal.str.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("fetch() SQL error - ") + sqlite3_errmsg(g_db));
        return;
    }

    int colCount = sqlite3_column_count(stmt);
    std::vector<std::string> fields;
    for (int i = 0; i < colCount; i++)
        fields.push_back(sqlite3_column_name(stmt, i));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::vector<Value> values;
        for (int i = 0; i < colCount; i++) {
            int type = sqlite3_column_type(stmt, i);
            Value val; val.isInitialized = true;
            if (type == SQLITE_INTEGER) { val.type=VAL_INT; val.num=sqlite3_column_int64(stmt,i); }
            else if (type == SQLITE_FLOAT) { val.type=VAL_FLOAT; val.decimal=sqlite3_column_double(stmt,i); }
            else { val.type=VAL_STR; val.str=(sqlite3_column_text(stmt,i)?(const char*)sqlite3_column_text(stmt,i):""); }
            values.push_back(val);
        }
        if (g_chainStore) {
            g_chainStore(vm, chainVal.str, values, fields);
        }
    }

    sqlite3_finalize(stmt);
}

// ── exists("tablename") ───────────────────────────────────────────────────────
void db_exists(AstraVM* vm) {
    Value v = vm->pop();
    if (!requireDb(vm)) { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); return; }
    if (v.type != VAL_STR || v.str.empty()) {
        if (g_reportError) g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "exists() expects a non-empty table name");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

   
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("exists() failed - ") + sqlite3_errmsg(g_db));
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }
    sqlite3_bind_text(stmt, 1, v.str.c_str(), -1, SQLITE_TRANSIENT);

    int exists = (sqlite3_step(stmt) == SQLITE_ROW) ? 1 : 0;
    sqlite3_finalize(stmt);
    vm->push(mkInt(exists));
}

// ── count("tablename") ────────────────────────────────────────────────────────
void db_count(AstraVM* vm) {
    Value v = vm->pop();
    if (!requireDb(vm)) { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); return; }
    if (v.type != VAL_STR || v.str.empty()) {
        if (g_reportError) g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "count() expects a non-empty table name");
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    std::string sql = "SELECT COUNT(*) FROM \"" + escapeIdentifier(v.str) + "\";";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        if (g_reportError) g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("count() failed - ") + sqlite3_errmsg(g_db));
        Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison);
        return;
    }

    long long count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    vm->push(mkInt(count));
}

// ── drop("tablename") ─────────────────────────────────────────────────────────
void db_drop(AstraVM* vm) {
    Value v = vm->pop();
    if (!requireDb(vm)) return;
    if (v.type != VAL_STR || v.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "drop() expects a non-empty table name");
        return;
    }

    std::string sql = "DROP TABLE IF EXISTS \"" + escapeIdentifier(v.str) + "\";";
    char* err = nullptr;
    int rc = sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            std::string("drop() failed - ") + (err ? err : "unknown error"));
        sqlite3_free(err);
    }
}

// ── escape("str") ──────────────────────────────────────────────────────────────
void db_escape(AstraVM* vm) {
    Value v = vm->pop();
    if (v.type != VAL_STR) {
        if (g_reportError) g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "escape() expects a string");
        Value poison; poison.isPoisoned = true; poison.type = VAL_STR; vm->push(poison);
        return;
    }
    std::string result;
    for (char c : v.str) {
        if (c == '\'') result += "''";
        else result += c;
    }
    vm->push(mkStr(result));
}

// ── lastid() ───────────────────────────────────────────────────────────────────
void db_lastid(AstraVM* vm) {
    if (!requireDb(vm)) { Value poison; poison.isPoisoned = true; poison.type = VAL_INT; vm->push(poison); return; }
    vm->push(mkInt((long long)sqlite3_last_insert_rowid(g_db)));
}

// ── bulkInsertChain("table", "chainName", "cond") ────────────────────────────
void db_bulkinsert_chain(AstraVM* vm) {
    Value condVal  = vm->pop();
    Value chainVal = vm->pop();
    Value tblVal   = vm->pop();

    if (!requireDb(vm)) return;
    if (tblVal.type != VAL_STR || tblVal.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "bulkinsert() expects a non-empty table name");
        return;
    }
    if (chainVal.type != VAL_STR || chainVal.str.empty()) {
        g_reportError(ErrCode::TYPE_MISMATCH, vm->currentLine, "bulkinsert() expects a chain name");
        return;
    }

    std::string chainName = chainVal.str;
    if (vm->chainTable.find(chainName) == vm->chainTable.end()) {
        g_reportError(ErrCode::CHAIN_NOT_FOUND, vm->currentLine, chainName);
        return;
    }

    ChainInfo& info = vm->chainTable[chainName];
    if (info.fields.empty()) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
            "bulkinsert() chain '" + chainName + "' has no named fields");
        return;
    }

    std::string cols;
    for (size_t f = 0; f < info.fields.size(); f++) {
        cols += "\"" + escapeIdentifier(info.fields[f]) + "\"";
        if (f < info.fields.size() - 1) cols += ",";
    }

    std::string condField, condValue, condOp;
    bool hasCondition = !condVal.str.empty();
    if (hasCondition) {
        const std::vector<std::string> ops = {">=", "<=", "!=", ">", "<", "="};
        size_t opPos = std::string::npos;
        std::string foundOp;
        for (auto& op : ops) {
            size_t pos = condVal.str.find(op);
            if (pos != std::string::npos) {
                opPos = pos;
                foundOp = op;
                break;
            }
        }
        if (opPos != std::string::npos) {
            condField = condVal.str.substr(0, opPos);
            condValue = condVal.str.substr(opPos + foundOp.size());
            condOp    = foundOp;
        } else {
            AstraError::syntax(ErrCode::INVALID_SYNTAX, 0,
                "bulkinsert() malformed condition '" + condVal.str + "'");
            hasCondition = false;
        }
    }

    if (sqlite3_exec(g_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "bulkinsert() could not start transaction");
        return;
    }
    char* err = nullptr;
    int failCount = 0;

    int totalObjs = !info.fields.empty() ? info.namedOffset : info.count;
    for (int t = 1; t <= totalObjs; t++) {
        std::string flat = chainName + std::to_string(t);

        if (hasCondition) {
            std::string key = flat + ":" + condField;
            auto it = vm->vmSymbolTable.find(key);
            if (it == vm->vmSymbolTable.end()) continue;

            Value& fv = vm->memory[it->second];
            bool match = false;

            if (fv.type == VAL_INT || fv.type == VAL_FLOAT) {
                double fnum = (fv.type == VAL_FLOAT) ? fv.decimal : (double)fv.num;
                double cnum;
                try { cnum = std::stod(condValue); }
                catch (...) { continue; }

                if      (condOp == "=")  match = (fnum == cnum);
                else if (condOp == "!=") match = (fnum != cnum);
                else if (condOp == ">")  match = (fnum >  cnum);
                else if (condOp == "<")  match = (fnum <  cnum);
                else if (condOp == ">=") match = (fnum >= cnum);
                else if (condOp == "<=") match = (fnum <= cnum);
            } else {
                std::string fstr = fv.str;
                if      (condOp == "=")  match = (fstr == condValue);
                else if (condOp == "!=") match = (fstr != condValue);
                else if (condOp == ">")  match = (fstr >  condValue);
                else if (condOp == "<")  match = (fstr <  condValue);
                else if (condOp == ">=") match = (fstr >= condValue);
                else if (condOp == "<=") match = (fstr <= condValue);
            }

            if (!match) continue;
        }

        std::string vals;
        for (size_t f = 0; f < info.fields.size(); f++) {
            std::string key = flat + ":" + info.fields[f];
            auto it = vm->vmSymbolTable.find(key);
            if (it == vm->vmSymbolTable.end()) { vals += "NULL"; }
            else {
                Value& v = vm->memory[it->second];
                if (v.type == VAL_INT)        vals += std::to_string(v.num);
                else if (v.type == VAL_FLOAT) vals += std::to_string(v.decimal);
                else {
                    std::string esc;
                    for (char c : v.str) { if (c == '\'') esc += "''"; else esc += c; }
                    vals += "'" + esc + "'";
                }
            }
            if (f < info.fields.size() - 1) vals += ",";
        }

        std::string sql = "INSERT INTO \"" + escapeIdentifier(tblVal.str) + "\" (" + cols + ") VALUES (" + vals + ");";
        int rc = sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            failCount++;
            g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine,
                "bulkinsert() failed at row " + std::to_string(t) + " - " + (err ? err : "unknown error"));
            sqlite3_free(err); err = nullptr;
        }
    }

    if (sqlite3_exec(g_db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        g_reportError(ErrCode::INVALID_OPERATION, vm->currentLine, "bulkinsert() commit failed");
    }
}

ASTRA_EXPORT void astra_set_chain(ChainStoreFn fn) {
    g_chainStore = fn;
}

// ── Register ─────────────────────────────────────────────────────────────────
ASTRA_EXPORT void astra_init(RegisterFunc reg) {
    reg("open",   db_open);
    reg("close",  db_close);
    reg("query",  db_query);
    reg("dbfetch", db_fetch);
    reg("exists", db_exists);
    reg("count",  db_count);
    reg("drop",   db_drop);
    reg("escape", db_escape);
    reg("lastid", db_lastid);
    reg("bulkinsert", db_bulkinsert_chain);
}

ASTRA_EXPORT const char* astra_logic(const char* cmd, const char* args) {
    if (std::string(cmd) == "info") {
        return
            "open(file)|Open/create SQLite database\n"
            "close()|Close database connection\n"
            "query(sql)|Execute SQL (INSERT,CREATE,UPDATE,DELETE)\n"
            "dbfetch(chain,sql)|SELECT results -> chain\n"
            "exists(table)|Check if table exists -> 1/0\n"
            "count(table)|Row count of table\n"
            "drop(table)|Delete table\n"
            "escape(str)|Escape string for SQL safety\n"
            "lastid()|Last inserted row ID\n"
            "bulkinsert(table,chain,cond)|Insert chain rows into table\n";
    }
    return "DB_Module_Active";
}
