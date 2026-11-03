#ifndef CHECKER_HPP
#define CHECKER_HPP

#include <string>
#include <vector>
#include "parser.hpp"

enum class VarType {
    Unique, NonUnique, Array
};

struct VarInfo {
    std::string name;
    VarType ptr_type;
};

enum class Current {
    None, Class, Struct,
    Function, Constructor, Loop, Module
};

struct Scope {
    Current current;
    std::string name;
};

class Checker {
public:
    void addVar(const std::string& name, VarType ptr_type);
    bool varExists(const std::string& name);
    VarType getPtrType(const std::string& name);

    void addClass(const std::string& name);
    bool classExists(const std::string& name);

    void addStruct(const std::string& name);
    bool structExists(const std::string& name);

    void addConstruct(const std::string& name);
    bool constructExists(const std::string& name);

    void updateScope(Current current, const std::string& name = "");
    bool isInScope(Current current, const std::string& name = "");

    void addSpecialScope(Current current, const std::string& name);
    bool checkSpecialScope(Current current, const std::string& name);
};

#endif