#pragma once

#include <unordered_map>
#include "value.hpp"
// useless
struct Runtime
{
    std::unordered_map<std::string, Value> variables;
};