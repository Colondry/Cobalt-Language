#include "checker.hpp"
#include <string>
#include <vector> 

std::vector<VarInfo> vptr;
std::vector<std::string> classes;
std::vector<std::string> structs;
std::vector<std::string> constructs;

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