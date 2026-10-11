#ifndef CHECKER_HPP
#define CHECKER_HPP

#include <string>
#include <vector>
#include "parser.hpp"

enum class VarType {
    Unique, NonUnique
};

struct VarInfo {
    std::string name;
    VarType ptr_type;
};

class Checker {
public:
    void addVar(const std::string& name, VarType ptr_type);
    bool varExists(const std::string& name);
    VarType getPtrType(const std::string& name);
};

#endif