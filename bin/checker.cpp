#include "checker.hpp"
#include <string>
#include <vector> 
std::vector<VarInfo> vptr;
VarType Checker::getPtrType(const std::string& name) {
    for (const auto& var : vptr) {
        if (var.name == name) {
            return var.ptr_type;
        }
    }
    return VarType::NonUnique; // Default to NonUnique if not found
}
void Checker::addVar(const std::string& name, VarType ptr_type) {
    vptr.push_back({name, ptr_type});
}
bool Checker::varExists(const std::string& name) {
    for (const auto& var : vptr) {
        if (var.name == name) {
            return true;
        }
    }
    return false;
}