#pragma once

#include "Value.hpp"

enum class ExecState
{
    Normal,
    Return,
    Break,
    Continue
};

struct ExecResult
{
    ExecState state = ExecState::Normal;
    Value value;
};