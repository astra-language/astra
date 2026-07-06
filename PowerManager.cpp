/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#include "PowerManager.h"
#include "vm.h"
#include "common.h"
#include <iostream>
#include <set>


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX        
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

static std::set<std::string> loadedModules;

static void chainStoreWrapper(void* vm,
                               const std::string& name,
                               std::vector<Value> values,
                               std::vector<std::string> fields) {
    ((AstraVM*)vm)->chainStore(name, values, fields);
}

void PowerManager::load(const std::string& fileName) {
    if (loadedModules.find(fileName) != loadedModules.end()) {
        std::cerr << ORANGE << "[ASTRA-WARN]" << RESET
                  << " :: " << YELLOW << " Astra Power Check" << RESET
                  << " :: " << CYAN << "'" << fileName << ".power'" << RESET
                  << " is already linked!" << std::endl;
        return;
    }

    std::string path = "powers/" + fileName + ".power";

#ifdef _WIN32
    HMODULE hLib = LoadLibraryA(path.c_str());
#else
    void* hLib = dlopen(path.c_str(), RTLD_LAZY);
#endif

    if (!hLib) {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            "Astra Power Check :: '" + fileName + ".power' Not Found or Failed to load");
        return;
    }

#ifdef _WIN32
    auto init = (void(*)(RegisterFunc))GetProcAddress(hLib, "astra_init");
#else
    auto init = (void(*)(RegisterFunc))dlsym(hLib, "astra_init");
#endif

    if (init) {
        init([](const char* name, void(*func)(AstraVM*)) {
            PowerManager::getInstance().registerFunction(name, func);
        });
         
        #ifdef _WIN32
    auto setChain = (void(*)(ChainStoreFn))GetProcAddress(hLib, "astra_set_chain");
#else
    auto setChain = (void(*)(ChainStoreFn))dlsym(hLib, "astra_set_chain");
#endif
    if (setChain) setChain(chainStoreWrapper);

        #ifdef _WIN32
    auto setError = (void(*)(ErrorReportFn))GetProcAddress(hLib, "astra_set_error");
#else
    auto setError = (void(*)(ErrorReportFn))dlsym(hLib, "astra_set_error");
#endif
    if (setError) setError(&AstraError::runtime);

        loadedModules.insert(fileName);
        handles[fileName] = (LibHandle)hLib;  

        std::cerr << MAGENTA << "[ASTRA-INFO]" << RESET
                  << " :: " << YELLOW << " Astra Power Check" << RESET
                  << " :: " << CYAN << "'" << fileName << ".power'" << RESET
                  << " Linked Successfully" << std::endl;
    } else {
        AstraError::runtime(ErrCode::INVALID_OPERATION, 0,
            "Could not find 'astra_init' in '" + fileName + "'");
    }
}

const char* PowerManager::queryInfo(const std::string& moduleName) {
    if (!handles.count(moduleName)) return nullptr;

    LibHandle lib = handles[moduleName];

#ifdef _WIN32
    auto fn = (const char*(*)(const char*, const char*))
              GetProcAddress((HMODULE)lib, "astra_logic");
#else
    auto fn = (const char*(*)(const char*, const char*))
              dlsym(lib, "astra_logic");
#endif

    if (!fn) return nullptr;
    return fn("info", "");
}