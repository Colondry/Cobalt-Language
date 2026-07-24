#pragma once
#include <string>

enum class ValueType {
    Void,
    Int,
    Float,
    Double,
    String,
    Bool,
    Char,
    Byte
};

struct Value {
    ValueType type = ValueType::Void;

    int intValue = 0;
    float floatValue = 0;
    double doubleValue = 0;
    std::string stringValue;
    bool boolValue = false;
    char charValue = '\0';
    unsigned char byteValue = 0;
};