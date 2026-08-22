#ifndef DEADCODE_HPP
#define DEADCODE_HPP

#include "ast.hpp"
#include <vector>
#include <string>

std::vector<StmtPtr> pruneUnusedVars(const std::vector<StmtPtr>& body, std::vector<std::string>& warnings);

#endif // DEADCODE_HPP