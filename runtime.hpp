#pragma once

#include <unordered_map>
#include "Value.hpp"

struct Runtime
{
    std::unordered_map<std::string, Value> variables;
};