#include "checker.hpp"
#include <string>
#include <vector> 

std::vector<VarInfo> vptr;
std::vector<std::string> classes;
std::vector<std::string> structs;
std::vector<std::string> constructs;
std::vector<Scope> special_scopes;
Scope current_scope{Current::None, ""};

VarType Checker::getPtrType(const std::string& name) {
    for (const auto& var : vptr) {
        if (var.name == name) return var.ptr_type;
    }
    return VarType::NonUnique;
}
void Checker::addVar(const std::string& name, VarType ptr_type) {
    vptr.push_back({name, ptr_type});
}
bool Checker::varExists(const std::string& name) {
    for (const auto& var : vptr) {
        if (var.name == name) return true;
    }
    return false;
}

void Checker::addClass(const std::string& name) {
    classes.push_back(name);
}

bool Checker::classExists(const std::string& name) {
    for (const auto& cls : classes) {
        if (cls == name) return true;
    }
    return false;
}

void Checker::addStruct(const std::string& name) {
    structs.push_back(name);
}
bool Checker::structExists(const std::string& name) {
    for (const auto& strct : structs) {
        if (strct == name) return true;
    }
    return false;
}

void Checker::addConstruct(const std::string& name) {
    constructs.push_back(name);
}
bool Checker::constructExists(const std::string& name) {
    for (const auto& c : constructs) {
        if (c == name) return true;
    }
    return false;
}

void Checker::updateScope(Current current, const std::string& name) {
    current_scope.current = current;
    current_scope.name = name;
}
bool Checker::isInScope(Current current, const std::string& name) {
    return current_scope.current == current && (name.empty() || current_scope.name == name);
}
void Checker::addSpecialScope(Current current, const std::string& name) {
    special_scopes.push_back({current, name});
}
bool Checker::checkSpecialScope(Current current, const std::string& name) {
    for (const auto& scope : special_scopes) {
        if (scope.current == current && scope.name == name) return true;
    }
    return false;
}