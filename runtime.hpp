#pragma once

#include <unordered_map>
#include "value.hpp"

struct Runtime
{
    std::unordered_map<std::string, Value> variables;
};