/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "file.h"
#include "common.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "error.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace fs = std::filesystem;

std::string FileOps::getSandboxDir(AstraVM* vm) {
    if (vm->sandboxDir.empty()) {
        vm->sandboxDir = fs::temp_directory_path().string() + "/astra_tmp_" + std::to_string(GETPID());
        fs::create_directories(vm->sandboxDir);
    }
    return vm->sandboxDir;
}

void FileOps::cleanupSandboxDirIfEmpty(AstraVM* vm) {
    if (vm->sandboxDir.empty()) return;
    std::error_code ec;
    if (fs::exists(vm->sandboxDir, ec) && fs::is_empty(vm->sandboxDir, ec)) {
        fs::remove(vm->sandboxDir, ec);
        vm->sandboxDir.clear();
    }
}

int FileOps::create(AstraVM* vm, const std::string& path) {
    std::string sandboxDir = getSandboxDir(vm);

    
    for (size_t i = 0; i < vm->fileHandles.size(); i++) {
        if (vm->fileHandles[i].isValid && vm->fileHandles[i].originalPath == path) {
            return (int)i;
        }
    }

    
    std::string sanitized = path;
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    std::string sandboxPath = sandboxDir + "/" + sanitized;

    std::error_code ec;
    if (fs::exists(path, ec)) {
        fs::copy_file(path, sandboxPath, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
                 " Cannot read '" + path + "': " + ec.message());
        }
    } else {
        std::ofstream ofs(sandboxPath);
        ofs.close();
    }

    FileHandle fh;
    fh.originalPath = path;
    fh.sandboxPath = sandboxPath;
    fh.isValid = true;
    fh.readLinePos = 0;
    
std::ifstream ifs(sandboxPath);
std::string line;
int count = 0;
while (std::getline(ifs, line)) count++;
fh.totalLines = count; 
ifs.close(); 

    vm->fileHandles.push_back(std::move(fh));
    return (int)vm->fileHandles.size() - 1;
}

void FileOps::close(AstraVM* vm, int handleId) {
    if (handleId < 0 || handleId >= (int)vm->fileHandles.size()) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid file handle");
    return;
    }
    FileHandle& fh = vm->fileHandles[handleId];
    if (fh.stream.is_open()) fh.stream.close();
    if (!fh.isValid) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "File handle already closed");
    return;
    }

    std::error_code ec;
    fs::copy_file(fh.sandboxPath, fh.originalPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
        "Cannot save '" + fh.originalPath + "': " + ec.message());
    }

    fs::remove(fh.sandboxPath, ec);
    fh.isValid = false;

    cleanupSandboxDirIfEmpty(vm);
}
void FileOps::plus(AstraVM* vm, int handleId, const std::string& text, const std::string& mode) {
    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || !vm->fileHandles[handleId].isValid) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid or closed file handle");
    return;
    }
    FileHandle& fh = vm->fileHandles[handleId];

    if (mode == "la") {
        
        std::ofstream ofs(fh.sandboxPath, std::ios::app);
        ofs << text << "\n";
        fh.totalLines++;
    }
    else if (mode == "fa") {
        
        std::ifstream ifs(fh.sandboxPath);
        std::string existing((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();

        std::ofstream ofs(fh.sandboxPath, std::ios::trunc);
        ofs << text << "\n" << existing;
        fh.totalLines++;
    }
    else if (mode == "raw") {
    
    std::ofstream ofs(fh.sandboxPath, std::ios::trunc);
    ofs << text;
    
    
    ofs.close();
    if (fh.stream.is_open()) {
        fh.stream.close();
    }
    std::ifstream ifs(fh.sandboxPath);
    std::string line;
    int count = 0;
    while (std::getline(ifs, line)) count++;
    fh.totalLines = count;
    fh.readLinePos = 0;
    }
    else {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "plus() invalid mode: '" + mode + "'");
    }
}

Value FileOps::readLine(AstraVM* vm, int handleId) {
    Value result;
    result.type = VAL_STR;
    result.str = "";
    result.isInitialized = true;

    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || !vm->fileHandles[handleId].isValid) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid or closed file handle");
        return result;
    }
    FileHandle& fh = vm->fileHandles[handleId];

    
    if (!fh.stream.is_open()) {
        fh.stream.open(fh.sandboxPath);
    }

    std::string line;
    if (std::getline(fh.stream, line)) {
        fh.readLinePos++;
        return stringToValue(line);
    }

    return result;
}

Value FileOps::readAll(AstraVM* vm, int handleId) {
    Value result;
    result.type = VAL_STR;
    result.str = "";
    result.isInitialized = true;

    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || !vm->fileHandles[handleId].isValid) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid or closed file handle");
    return result;
    }
    FileHandle& fh = vm->fileHandles[handleId];

    std::ifstream ifs(fh.sandboxPath);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    result.str = content;
    return result; 
}

bool FileOps::isEof(AstraVM* vm, int handleId) {
    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || 
        !vm->fileHandles[handleId].isValid) return true;
    
    
    return vm->fileHandles[handleId].readLinePos >= vm->fileHandles[handleId].totalLines;
}

Value FileOps::fetch(AstraVM* vm, int handleId, const std::string& mode, int pos1, int pos2) {
    Value result;
    result.type = VAL_STR;
    result.str = "";
    result.isInitialized = true;

    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || 
        !vm->fileHandles[handleId].isValid) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "fetch() invalid file handle");
        return result;
    }
    
    FileHandle& fh = vm->fileHandles[handleId];
    std::ifstream ifs(fh.sandboxPath);

    if (mode == "c") {
        
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        if (pos1 >= 1 && pos1 <= (int)content.size())
            result.str = content.substr(0, pos1);
    }
    else if (mode == "l") {
        
        std::string line;
        int current = 1;
        while (std::getline(ifs, line)) {
            if (current == pos1) { result.str = line; break; }
            current++;
        }
    }
    else if (mode == "lc") {
        
        std::string line;
        int current = 1;
        while (std::getline(ifs, line)) {
            if (current == pos1) {
                if (pos2 >= 1 && pos2 <= (int)line.size())
                    result.str = line.substr(0, pos2);
                else
                    result.str = line; 
                break;
            }
            current++;
        }
    }
    else if (mode == "count") {
    std::string line;
    int count = 0;
    while (std::getline(ifs, line)) count++;
    result.type = VAL_INT;
    result.num = count;
    result.isInitialized = true;
}
    return result;
}
void FileOps::clear(AstraVM* vm, int handleId) {
    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || 
    !vm->fileHandles[handleId].isValid) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid or closed file handle");
    return;
    }
    FileHandle& fh = vm->fileHandles[handleId];
    
    
    std::ofstream ofs(fh.sandboxPath, std::ios::trunc);
    ofs.close();
    if (fh.stream.is_open()) fh.stream.close();
    fh.totalLines = 0; 
    fh.readLinePos = 0;
}

void FileOps::clearLine(AstraVM* vm, int handleId, const std::string& text) {
    if (handleId < 0 || handleId >= (int)vm->fileHandles.size() || 
    !vm->fileHandles[handleId].isValid) {
    AstraError::runtime(ErrCode::INVALID_OPERATION, 0, "Invalid or closed file handle");
    return;
    }
    FileHandle& fh = vm->fileHandles[handleId];

    
    std::ifstream ifs(fh.sandboxPath);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find(text) == std::string::npos) {
            lines.push_back(line);
        }
    }
    ifs.close();

    std::ofstream ofs(fh.sandboxPath, std::ios::trunc);
    for (int i = 0; i < (int)lines.size(); i++) {
        ofs << lines[i];
        if (i < (int)lines.size() - 1) ofs << "\n";
    }
    ofs.close();
    if (fh.stream.is_open()) fh.stream.close();
    
    fh.totalLines = (int)lines.size();
    
    
    if (fh.readLinePos > fh.totalLines) {
        fh.readLinePos = fh.totalLines;
    }
}