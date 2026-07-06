/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

 
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <map>
#include <string>
#include "astra_sdk.h"


typedef void* LibHandle;

class AstraVM;

class PowerManager {
public:
    std::map<std::string, void(*)(AstraVM*)> registry;
    std::map<std::string, LibHandle>         handles;

    static PowerManager& getInstance() {
        static PowerManager instance;
        return instance;
    }

    void load(const std::string& fileName);

    void registerFunction(std::string name, void(*func)(AstraVM*)) {
        registry[name] = func;
    }

    const char* queryInfo(const std::string& moduleName);
};

#endif