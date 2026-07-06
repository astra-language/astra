/*
 * Astra Programming Language
 * Copyright (c) 2026 Rajanala Vijay Kumar
 *
 * Licensed under the MIT License. See the LICENSE file in the
 * project root for full license text.
 */

#pragma once
#include "vm.h"

namespace BuiltinFunctions {
    void chainLen(AstraVM* vm, const std::string& chainName);
    void chainSort(AstraVM* vm, const std::string& chainName);
    void chainMerge(AstraVM* vm, const std::string& chain1, const std::string& chain2);
    void chainUnique(AstraVM* vm, const std::string& chainName);
    void chainSum(AstraVM* vm, const std::string& chainName);
    void chainAvg(AstraVM* vm, const std::string& chainName);
    void chainMax(AstraVM* vm, const std::string& chainName);
    void chainMin(AstraVM* vm, const std::string& chainName);
    void chainReverse(AstraVM* vm, const std::string& chainName);
    void chainContains(AstraVM* vm, const std::string& chainName, const Value& target);
    void chainIndexOf(AstraVM* vm, const std::string& chainName, const Value& target);
    void chainJoin(AstraVM* vm, const std::string& chainName, const std::string& delim);
    void adr(AstraVM* vm, const std::string& varName);
    void val(AstraVM* vm, Value addrVal);
    void jsonPretty(AstraVM* vm, const std::string& jsonStr);
}