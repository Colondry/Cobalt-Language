#pragma once

#include "value.hpp"
// useless
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