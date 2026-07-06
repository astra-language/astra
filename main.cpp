/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <csignal>
#include <cstdlib>
#include "lexer.h"
#include "compiler.h"
#include "vm.h"
#include "parser.h"
#include "error.h"

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#else
    #include <termios.h>
    #include <unistd.h>
#endif

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define RED     "\033[31m"
#define BLUE    "\033[34m"

// ─── Signal Handler ───────────────────────────────────────────────────────────

void signalHandler(int signum) {
    std::cout << "\n" << RED << "Exiting Astra..." << RESET << "\n";
    exit(signum);
}

// ─── ANSI Enable (Windows only) ───────────────────────────────────────────────

void enableAnsi() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    SetConsoleOutputCP(CP_UTF8);
#endif
}

// ─── Keyword Coloring ─────────────────────────────────────────────────────────

void printColoredLine(const std::string& line) {
    size_t first = line.find_first_not_of(' ');
    if (first == std::string::npos) {
        std::cout << line;
        return;
    }

    std::string trimmed = line.substr(first);
    std::string spaces  = line.substr(0, first);
    std::cout << spaces;

    const char* keywords[] = {
        "else if ", "writes ", "write ", "if ", "else ",
        "exit ", "repeat ", "add ", "#f ", "#ef ", "const ", nullptr
    };

    for (int i = 0; keywords[i]; ++i) {
        std::string kw(keywords[i]);
        if (trimmed.rfind(kw, 0) == 0) {
            std::cout << YELLOW << kw << RESET << trimmed.substr(kw.length());
            return;
        }
    }
    std::cout << trimmed;
}

// ─── Prompt Builder ───────────────────────────────────────────────────────────

void printPrompt(int nestLevel, int lineCounter) {
    if (nestLevel > 0)
        std::cout << BLUE << "block[" << nestLevel << "] >> " << RESET << std::flush;
    else
        std::cout << GREEN << "Astra " << YELLOW
                  << "[" << lineCounter << "] >> " << RESET << std::flush;
}

// Prompt string (for redraw width calculation)
std::string promptString(int nestLevel, int lineCounter) {
    if (nestLevel > 0)
        return "block[" + std::to_string(nestLevel) + "] >> ";
    return "Astra [" + std::to_string(lineCounter) + "] >> ";
}

// ─── Redraw Current Line ──────────────────────────────────────────────────────

void redrawLine(int nestLevel, int lineCounter, const std::string& line) {
    // Wipe entire line then reprint
    std::string ps = promptString(nestLevel, lineCounter);
    // Move to column 0, overwrite with spaces, move back
    std::cout << "\r" << std::string(ps.length() + line.length() + 4, ' ') << "\r";
    printPrompt(nestLevel, lineCounter);
    printColoredLine(line);
    std::cout << std::flush;
}

// ─── Platform Input ───────────────────────────────────────────────────────────

#ifndef _WIN32
// Linux: enable raw mode once per REPL loop, not per character
static struct termios g_oldt;
static bool g_rawModeActive = false;

void rawModeOn() {
    tcgetattr(STDIN_FILENO, &g_oldt);
    struct termios newt = g_oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN]  = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    g_rawModeActive = true;
}

void rawModeOff() {
    if (g_rawModeActive) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_oldt);
        g_rawModeActive = false;
    }
}
#endif

// ─── VM Runner ────────────────────────────────────────────────────────────────

void runAstra(const std::string& source, AstraVM& vm, Compiler& compiler, int currentLine) {
    Lexer lexer(source);
    lexer.setLine(currentLine);
    auto tokens = lexer.tokenize();

    compiler.syncFromVM(vm.vmSymbolTable, vm.vmNextVarId, vm.varNames);

    auto bytecode = compiler.compile(tokens);
    if (bytecode.empty() || bytecode.size() <= 1) return;

    vm.setVarNames(compiler.varNames);
    vm.setStringPool(compiler.stringPool);
    vm.setFloatPool(compiler.floatPool);
    vm.setLongPool(compiler.longPool);
    vm.functionTable = &compiler.functionTable;
    vm.modifierTable = compiler.modifierTable;
    vm.hasDynamicResolution = compiler.hasDynamicResolution;
    
    int maxId = 0;
    for (auto& pair : compiler.symbolTable) {
        vm.vmSymbolTable[pair.first] = pair.second;
        if (pair.second >= maxId) maxId = pair.second + 1;
    }
    if (maxId > vm.vmNextVarId) vm.vmNextVarId = maxId;

    vm.reportedErrors.clear();
    vm.execute(bytecode);
}

// ─── Read One Line (cross-platform, with history + arrow keys) ────────────────

std::string readLine(int nestLevel, int lineCounter,
                     std::vector<std::string>& history, int& historyIndex)
{
    std::string line;

#ifdef _WIN32
    // ── Windows ──────────────────────────────────────────────────────────────
    while (true) {
        int ch = _getch();

        if (ch == 3) signalHandler(SIGINT);   // Ctrl+C

        if (ch == '\r' || ch == '\n') {        // Enter
            std::cout << "\n";
            break;
        }

        if (ch == 8) {                         // Backspace
            if (!line.empty()) {
                line.pop_back();
                redrawLine(nestLevel, lineCounter, line);
            }
            continue;
        }

        // Arrow / special keys: _getch() returns 0 or 224 as prefix
        if (ch == 0 || ch == 224) {
            int arrow = _getch();
            if (arrow == 72) {                 // Up
                if (historyIndex < (int)history.size() - 1) historyIndex++;
                if (historyIndex >= 0)
                    line = history[history.size() - 1 - historyIndex];
            } else if (arrow == 80) {          // Down
                if (historyIndex > 0) {
                    historyIndex--;
                    line = history[history.size() - 1 - historyIndex];
                } else {
                    historyIndex = -1;
                    line.clear();
                }
            }
            // All other special keys silently consumed above
            redrawLine(nestLevel, lineCounter, line);
            continue;
        }

        if (ch >= 32 && ch < 127) {            // Printable ASCII
            line += static_cast<char>(ch);
            redrawLine(nestLevel, lineCounter, line);
        }
    }

#else
    // ── Linux / macOS ─────────────────────────────────────────────────────────
    rawModeOn();

    while (true) {
        unsigned char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) <= 0) continue;

        if (ch == 3) {                         // Ctrl+C
            rawModeOff();
            signalHandler(SIGINT);
        }

        if (ch == '\n' || ch == '\r') {        // Enter
            rawModeOff();
            std::cout << "\n";
            break;
        }

        if (ch == 127 || ch == 8) {            // Backspace
            if (!line.empty()) {
                line.pop_back();
                redrawLine(nestLevel, lineCounter, line);
            }
            continue;
        }

        if (ch == 27) {                        // ESC sequence
            unsigned char seq[2] = {0, 0};
            // Use read() with timeout-safe approach
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') {           // Up Arrow
                    if (historyIndex < (int)history.size() - 1) historyIndex++;
                    if (historyIndex >= 0)
                        line = history[history.size() - 1 - historyIndex];
                } else if (seq[1] == 'B') {    // Down Arrow
                    if (historyIndex > 0) {
                        historyIndex--;
                        line = history[history.size() - 1 - historyIndex];
                    } else {
                        historyIndex = -1;
                        line.clear();
                    }
                }
                // Left/Right/Delete etc. silently consumed
            }
            redrawLine(nestLevel, lineCounter, line);
            continue;
        }

        if (ch >= 32 && ch < 127) {            // Printable ASCII
            line += static_cast<char>(ch);
            redrawLine(nestLevel, lineCounter, line);
            continue;
        }

        // Multi-byte UTF-8 passthrough (for Telugu characters etc.)
        if (ch >= 0x80) {
            // Determine byte count from leading byte
            int extra = 0;
            if      ((ch & 0xE0) == 0xC0) extra = 1;
            else if ((ch & 0xF0) == 0xE0) extra = 2;
            else if ((ch & 0xF8) == 0xF0) extra = 3;

            std::string mb;
            mb += static_cast<char>(ch);
            for (int i = 0; i < extra; ++i) {
                unsigned char b = 0;
                if (read(STDIN_FILENO, &b, 1) > 0) mb += static_cast<char>(b);
            }
            line += mb;
            redrawLine(nestLevel, lineCounter, line);
        }
    }

#endif

    return line;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    signal(SIGINT, signalHandler);
#ifndef _WIN32
    std::atexit(rawModeOff);
#endif
    enableAnsi();
    
    AstraVM* vmPtr = new AstraVM();
    AstraVM& vm = *vmPtr;
    Compiler compiler;
    int lineCounter = 1;

    // ── File mode ─────────────────────────────────────────────────────────────
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << RED << "Error: Could not open file "
                      << argv[1] << RESET << "\n";
            return 1;
        }
        std::stringstream buf;
        buf << file.rdbuf();
        runAstra(buf.str(), vm, compiler, 1);
        delete vmPtr;
        return 0;
    }

    // ── Interactive (REPL) mode ───────────────────────────────────────────────
    std::cout << CYAN << "Astra VM Ready (Type 'exit' to quit)" << RESET << "\n";

    std::vector<std::string> history;
    int historyIndex = -1;
    int nestLevel    = 0;
    std::string buffer;

    auto containsKeyword = [](const std::string& line, const std::string& kw) {
    size_t pos = line.find(kw);
    while (pos != std::string::npos) {
        bool beforeOk = (pos == 0 || !isalnum(line[pos - 1]));
        bool afterOk  = (pos + kw.size() >= line.size() || !isalnum(line[pos + kw.size()]));
        if (beforeOk && afterOk) return true;
        pos = line.find(kw, pos + 1);
    }
    return false;
    };

    while (true) {
        printPrompt(nestLevel, lineCounter);
        std::string line = readLine(nestLevel, lineCounter, history, historyIndex);

        if (line == "exit") break;
        if (line.empty())   continue;

        history.push_back(line);
        historyIndex = -1;

        // ── 'info' command ────────────────────────────────────────────────────
        if (line.find("info") != std::string::npos) {
            runAstra(line, vm, compiler, lineCounter++);
            continue;
        }

        // ── Function definition start: #f ─────────────────────────────────────
        if (line.find("#f") != std::string::npos) {
            nestLevel++;
            buffer += line + "\n";
            continue;
        }

        // ── Function definition end: #ef ──────────────────────────────────────
        if (line.find("#ef") != std::string::npos) {
            buffer += line + "\n";
            if (--nestLevel == 0) {
                runAstra(buffer, vm, compiler, lineCounter++);
                buffer.clear();
            }
            continue;
        }

        // ── Block start: if / repeat ──────────────────────────────────────────
        if (containsKeyword(line, "if") || containsKeyword(line, "repeat"))
        {
            Lexer  tempLexer(line);
            Parser tempParser(tempLexer.tokenize());

            if (!tempParser.isBlockLineValid(line)) {
                std::string ctx = nestLevel > 0
                    ? " [block " + std::to_string(nestLevel) + "]" : "";
                AstraError::syntax(ErrCode::INVALID_SYNTAX, lineCounter,
                                   " after '" + line + "'" + ctx);
                continue;
            }

            nestLevel++;
            buffer += line + "\n";
            continue;
        }

        // ── Block end: ; ──────────────────────────────────────────────────────
        if (line.find(";") != std::string::npos) {
            buffer += line + "\n";
            if (--nestLevel == 0) {
                runAstra(buffer, vm, compiler, lineCounter++);
                buffer.clear();
            }
            continue;
        }

        // ── Inside a block ────────────────────────────────────────────────────
        if (nestLevel > 0) {
            buffer += line + "\n";
            continue;
        }

        // ── Normal single line ────────────────────────────────────────────────
        runAstra(line, vm, compiler, lineCounter++);
    }
    delete vmPtr;
    return 0;
}