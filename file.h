/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#ifndef FILE_H
#define FILE_H

#include "vm.h"
#include <string>

namespace FileOps {
    int create(AstraVM* vm, const std::string& path);
    void close(AstraVM* vm, int handleId);
    std::string getSandboxDir(AstraVM* vm);
    void cleanupSandboxDirIfEmpty(AstraVM* vm);
    void plus(AstraVM* vm, int handleId, const std::string& text, const std::string& mode);
    Value readLine(AstraVM* vm, int handleId);  
    Value readAll(AstraVM* vm, int handleId);   
    
    bool isEof(AstraVM* vm, int handleId);
    Value fetch(AstraVM* vm, int handleId, const std::string& mode, int pos1, int pos2 = 0);
    void clear(AstraVM* vm, int handleId);
    void clearLine(AstraVM* vm, int handleId, const std::string& text);

}

#endif