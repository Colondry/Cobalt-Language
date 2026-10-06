#include "codeGen.hpp"
#include "flib.hpp"
#include "Deadcode.hpp"
#include "vgen.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <unordered_set>

namespace fs = std::filesystem;

static std::vector<StmtPtr> pruneAndReport(const std::vector<StmtPtr>& body, const std::string& where) {
    std::vector<std::string> warnings;
    std::vector<StmtPtr> pruned = pruneUnusedVars(body, warnings);
    for (const std::string& w : warnings) {
        std::cerr << "warning: " << w << " (in " << where << ")\n";
    }
    return pruned;
}

static std::string emitTypeDeclLine(const TypeDecl& t) {
    if (t.type == "List") {
        return "using " + t.name + " = std::vector<" + cppType(t.elemType) + ">;\n";
    }
    if (t.type == "Fraction") {
        return "using " + t.name + " = frac<" + cppType(t.elemType) + ", " + cppType(t.secElemType) + ">;\n";
    }
    if (t.arraySize >= 0) {
        return "using " + t.name + " = " + cppType(t.type) + "[" + std::to_string(t.arraySize) + "];\n";
    }
    return "using " + t.name + " = " + cppType(t.type) + ";\n";
}

static std::string emitConcatPieces(const std::shared_ptr<ConcatExpr>& c) {
    std::string out; CodeGenVisitor v;
    for (size_t i = 0; i < c->pieces.size(); i++) {
        if (i) out += " << ";
        out += v.emitExpr(c->pieces[i]);
    }
    return out;
}

static std::string emitCFDSignature(const CFDecl& fn) {
    std::string out = cppType(fn.returnType) + " " + fn.name + "(";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        out += cppType(fn.params[i].type) + " " + fn.params[i].name;
    }
    return out += ")";
}

static std::string emitLambSignature(const LambFuncDecl& fn) {
    std::string out = "auto " + fn.name + " = [](";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        out += cppType(fn.params[i].type) + " " + fn.params[i].name;
    }
    return out += ") -> " + cppType(fn.returnType);
}

static std::string emitCFSignature(const CFuncDecl& fn) {
    std::string out = cppType(fn.returnType) + " " + fn.name + "(";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        out += cppType(fn.params[i].type) + " " + fn.params[i].name;
    }
    return out += ")";
}

static std::string emitSignature(const FunctionDecl& fn, const bool isDecl = false) {
    std::string out = (fn.inl ? "inline " : "") + cppType(fn.returnType) + " " + fn.name + "(";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        if (!fn.params[i].isNotPointer) out += "std::unique_ptr<" + cppType(fn.params[i].type, "param") + "> " + fn.params[i].name;
        else out += cppType(fn.params[i].type, "param") + " " + fn.params[i].name;
        if (fn.params[i].arraySize >= 0) out += "[" + std::to_string(fn.params[i].arraySize) + "]";
        if (isDecl) {
            if (!fn.params[i].isNotPointer) out += " = std::make_unique<" + cppType(fn.params[i].type, "param") + ">(" + CodeGenVisitor().emitExpr(fn.params[i].init, fn.params[i].isNotPointer, fn.params[i].type) + ")";
            else if (fn.params[i].init) out += " = " + CodeGenVisitor().emitExpr(fn.params[i].init, fn.params[i].isNotPointer, fn.params[i].type);
        }
    }
    return out += ")";
}

void codeGen(Program& program, std::string fileName, const std::string& inputFileDir) {
    std::ofstream file(fileName + ".cpp");
    if (!file) { std::cerr << "Cannot create " << (fileName + ".cpp") << ".\n"; std::exit(EXIT_FAILURE); }

    file << "#include <iostream>\n";
    file << "#include <vector>\n";
    file << "#include <cstdint>\n";
    file << "#include <cmath>\n";   // std::pow, emitted for the '^' operator
    file << "#include <stdfloat>\n\n";
    file << "#include <utility>\n";
    file << "#include <memory>\n";
    file << "#include <type_traits>\n";
    file << "#include <cnow.hpp>\n";
    file << "#include <cstart.hpp>\n\n";

    file << "inline void syncw_stdio(bool s) {\n";
    file << "   std::ios_base::sync_with_stdio(s);\n";
    file << "}\n";
    file << "inline thread_local int cobalt__try_status__ = 0;\n";
    file << "inline int TryStatus() { return cobalt__try_status__; }\n";

    file << "\n";

    std::unordered_set<std::string> libraryProvidedGlobals;
    static const std::regex externGlobalRe(R"(extern\s+__[A-Za-z0-9_]+__\s+([A-Za-z0-9_]+)\s*;)");
    auto scanFileForExternGlobals = [&](const fs::path& path) {
        std::ifstream in(path);
        if (!in) return;
        std::stringstream ss;
        ss << in.rdbuf();
        std::string text = ss.str();
        for (std::sregex_iterator it(text.begin(), text.end(), externGlobalRe), end; it != end; ++it) {
            libraryProvidedGlobals.insert((*it)[1].str());
        }
        };

    file << "\n";
    CodeGenVisitor cg;
    for (const auto& obj : program.usedObjects) {
        bool alreadyDeclared = false;
        // Is the name of a class already declared?
        for (const auto& decl : program.declarations) {
            if (auto* cls = std::get_if<ClassDecl>(&decl)) {
                if (cls->name == obj) {
                    alreadyDeclared = true;
                    break;
                }
            }
        }
        // Is it provided by a library?
        if (libraryProvidedGlobals.count(obj)) alreadyDeclared = true;
        // Only emit if it's not already handled.
        if (!alreadyDeclared) file << obj << " " << obj << ";\n";
    }

    for (auto& decl : program.declarations) {
        std::visit([&](auto&& d) {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, CFuncDecl>) {
                file << emitCFSignature(d) << " {\n" << cg.emitBlock(pruneAndReport(d.body, "function '" + d.name + "'"), 1) << "}\n\n";
            } else if constexpr (std::is_same_v<T, FunctionDecl>) {
                file << emitSignature(d) << " {\n";
                if (d.name == "main") file << indent(1) << "syncw_stdio(false);\n";
                file << cg.emitBlock(pruneAndReport(d.body, "function '" + d.name + "'"), 1) << "}\n\n";
            }
            else if constexpr (std::is_same_v<T, TypeDecl>) {
                file << cg.emitStmt(std::make_shared<TypeDecl>(d), 0);
            }
            else if constexpr (std::is_same_v<T, ClassDecl>) {
                file << "class " << d.name << " {\n";
                if (d.pub) file << "public:\n" << cg.emitBlock(d.publicBody, 1);
                if (d.pvr) file << "private:\n" << cg.emitBlock(d.privateBody, 1);
                file << "};\n";
            }
            else if constexpr (std::is_same_v<T, StructCode>) {
                file << "struct " << d.name << " {\n" << cg.emitBlock(d.body, 1) << "};\n";
            }
            else if constexpr (std::is_same_v<T, ModuleDecl>) {
                file << "namespace " << d.name << " {\n";
                file << cg.emitBlock(d.body, 1);
                file << "}\n";
            }
            else if constexpr (std::is_same_v<T, Use>) {
                if (d.mode == 0) file << d.first << " " << d.second << ";\n";
                else file << "namespace " << d.first << " = " << d.second << ";\n";
            }
            else if constexpr (std::is_same_v<T, AutoUse>) {
                if (d.mode == 0) [[unlikely]] file << d.libName << " " << d.libName << ";\n";
                else file << "using namespace " << d.libName << ";\n";
            }
            else if constexpr (std::is_same_v<T, LibImport>) {
                std::string headerPath;
                std::string bundleDir = findLibraryDir(d.libName, inputFileDir);
                if (!bundleDir.empty()) {
                    fs::path hpp = fs::path(bundleDir) / (d.libName + ".hpp");
                    fs::path h = fs::path(bundleDir) / (d.libName + ".h");
                    std::error_code ec;
                    if (fs::exists(hpp, ec) && !ec) headerPath = hpp.string();
                    else if (fs::exists(h, ec) && !ec) headerPath = h.string();
                }
                if (headerPath.empty() && d.libName != "std::csm") file << "#include \"" << d.libName << ".hpp\"\n";
                else if (!headerPath.empty() && d.libName != "std::csm") file << "#include \"" << fs::path(headerPath).generic_string() << "\"\n";
            }
            else if constexpr (std::is_same_v<T, CSMDecl>) {
                file << "#include <csystem.hpp>\n";
                file << "#include <cotype.hpp>\n";
                file << "#include <fsys.hpp>\n";
                file << "#include <errors.hpp>\n";
                file << "#include <runtime.hpp>\n";
                file << "#include <inf.hpp>\n";
                file << "#include <cstr.hpp>\n";
                file << "#include <fstream>\n";
                file << "#include <cstdio>\n";
            }
        }, decl);
    }
}