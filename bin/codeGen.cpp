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

static std::string cppType(const std::string& t, const std::string& context = "") {
    if (t == "string") return "std::string";
    if (t == "byte") return "__byte__";
    if (t == "std::uint8_t") return "__byte__"; // uint8_t == unsigned char; stream it as a number, not a glyph
    return t;
}

static std::string indent(int depth) { return std::string(depth * 4, ' '); }

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

static void emitPrintStmt(const ExprPtr& value, bool newline, int depth, std::ofstream& out) {
    out << indent(depth) << "std::cout";
    CodeGenVisitor v;
    if (value) {
        if (auto c = std::dynamic_pointer_cast<ConcatExpr>(value)) {
            out << " << " << emitConcatPieces(c);
        }
        else {
            out << " << " << v.emitExpr(value);
        }
    }
    if (newline) out << " << \"\\n\"";
    out << ";\n";
}
static void emitPrintMacStmt(const ExprPtr& value, bool newline, int depth, std::ofstream& out) {
    out << indent(depth) << (newline ? "println_c(" : "print_c(");
    CodeGenVisitor v;
    if (value) {
        if (auto c = std::dynamic_pointer_cast<ConcatExpr>(value); c && !c->pieces.empty()) {
            out << v.emitExpr(c->pieces[0]);
            for (size_t i = 1; i < c->pieces.size(); i++) {
                out << ", " << v.emitExpr(c->pieces[i]);
            }
        }
        else {
            out << v.emitExpr(value);
        }
    }
    out << ");\n";
}

static void emitContinueStmt(int depth, std::ofstream& out) {
    out << indent(depth) << "continue;\n";
}

static void emitBreakStmt(int depth, std::ofstream& out) {
    out << indent(depth) << "break;\n";
}

static void emitClearStmt(int depth, std::ofstream& out) {
#ifdef _WIN32
    out << indent(depth) << "system(\"cls\");\n";
#else
    out << indent(depth) << "std::cout << \"\\033[2J\\033[H\";\n";
    out << indent(depth) << "std::cout.flush();\n";
#endif
}


static std::string emitSignature(const FunctionDecl& fn) {
    std::string out = cppType(fn.returnType) + " " + fn.name + "(";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        out += cppType(fn.params[i].type, "param") + " " + fn.params[i].name;
    }
    return out += ")";
}

void codeGen(Program& program, std::string fileName, const std::string& inputFileDir) {
    std::ofstream file(fileName + ".cpp");
    if (!file) { std::cerr << "Cannot create " << (fileName + ".cpp") << ".\n"; std::exit(EXIT_FAILURE); }

    file << "#include <iostream>\n";
    file << "#include <vector>\n";
    file << "#include <cstdint>\n";
    file << "#include <stdfloat>\n\n";
    file << "#include <utility>\n";
    file << "#include <memory>\n";
    file << "#include <type_traits>\n";
    file << "#include <cnow.hpp>\n\n";

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

    for (const LibImport& imp : program.imports) {
        std::string headerPath;

        std::string bundleDir = findLibraryDir(imp.libName, inputFileDir);
        if (!bundleDir.empty()) {
            fs::path hpp = fs::path(bundleDir) / (imp.libName + ".hpp");
            fs::path h = fs::path(bundleDir) / (imp.libName + ".h");
            std::error_code ec;
            if (fs::exists(hpp, ec) && !ec) headerPath = hpp.string();
            else if (fs::exists(h, ec) && !ec) headerPath = h.string();

            std::error_code dirEc;
            for (const auto& entry : fs::directory_iterator(bundleDir, dirEc)) {
                if (dirEc) break;
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension();
                if (ext == ".hpp" || ext == ".h") scanFileForExternGlobals(entry.path());
            }
        }
        if (headerPath.empty()) headerPath = findLibraryFile(imp.libName, ".hpp", inputFileDir);
        if (!headerPath.empty() && bundleDir.empty()) scanFileForExternGlobals(headerPath);

        if (headerPath.empty()) file << "#include \"" << imp.libName << ".hpp\"\n";
        else file << "#include \"" << fs::path(headerPath).generic_string() << "\"\n";
    }
    if (program.use_built) { // not using '!use csm'
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
    file << "\n";
    CodeGenVisitor cg;
    for (const TypeDecl& td : program.typedefs) {
        file << cg.emitStmt(std::make_shared<TypeDecl>(td), 0);
    }
    for (const ModuleDecl& module : program.modules) {
        file << "namespace " << module.name << " {\n";
        file << cg.emitBlock(module.body, 1);
        file << "}\n";
    }

    for (const ClassDecl& cls : program.classes) {
        file << "class " << cls.name << " {\n";
        if (cls.pub) file << "public:\n" << cg.emitBlock(cls.publicBody, 1);
        if (cls.pvr) file << "private:\n" << cg.emitBlock(cls.privateBody, 1);
        file << "};\n";
    }
    for (const StructCode& str : program.struc) file << "struct " << str.name << " {\n" << cg.emitBlock(str.body, 1) << "};\n";
    for (const ClassDecl& cls : program.classes) file << cls.name << " " << cls.name << ";\n";
    for (const AutoUse& au : program.autouses) {
        if (au.mode == 0) [[unlikely]] file << au.libName << " " << au.libName << ";\n";
        else file << "using namespace " << au.libName << ";\n";
    }
    file << "\n";

    for (const Use& u : program.uses) {
        if (u.mode == 0) file << u.first << " " << u.second << ";\n";
        else file << "namespace " << u.first << " = " << u.second << ";\n";
    }
    file << "\n";
    for (const FunctionDecl& fn : program.functions) file << emitSignature(fn) << ";\n";
    file << "\n";
    for (auto& obj : program.usedObjects)
    {
        bool alreadyDeclared = false;
        for (const ClassDecl& cls : program.classes) {
            if (cls.name == obj) { alreadyDeclared = true; break; }
        }
        if (libraryProvidedGlobals.count(obj)) alreadyDeclared = true;
        if (!alreadyDeclared) {
            file << obj << " " << obj << ";\n";
        }
    }

    for (const CFuncDecl& cfnd : program.cfunctions) {
        file << emitCFSignature(cfnd) << " {\n" << cg.emitBlock(pruneAndReport(cfnd.body, "function '" + cfnd.name + "'"), 1) << "}\n\n";
    }

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << " {\n";
        if (fn.name == "main") file << indent(1) << "syncw_stdio(false);\n";
        file << cg.emitBlock(pruneAndReport(fn.body, "function '" + fn.name + "'"), 1) << "}\n\n";
    }
}