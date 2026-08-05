#include "codeGen.hpp"
#include "flib.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <unordered_set>

namespace fs = std::filesystem;

static std::string cppType(const std::string& t) {
    if (t == "string") return "std::string";
    if (t == "byte") return "uint8_t";
    return t;
}

static std::string indent(int depth) { return std::string(depth * 4, ' '); }

static std::string emitExpr(const ExprPtr& e);

static std::string emitConcatPieces(const std::shared_ptr<ConcatExpr>& c) {
    std::string out;
    for (size_t i = 0; i < c->pieces.size(); i++) {
        if (i) out += " << ";
        out += emitExpr(c->pieces[i]);
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

static std::string emitExpr(const ExprPtr& e) {
    if (auto n = std::dynamic_pointer_cast<NumberLit>(e)) return n->value;
    if (auto s = std::dynamic_pointer_cast<StringLit>(e)) return s->value;
    if (auto c = std::dynamic_pointer_cast<CharLit>(e)) return c->value;
    if (auto id = std::dynamic_pointer_cast<NameExpr>(e)) return id->name;
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) return "(-" + emitExpr(u->operand) + ")";
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) {
        if (b->op == "+") {
            // Plain C++ `+` doesn't compile for e.g. int + const char*
            // (`height + "cm"`) -- __cobalt_add__ picks string-concat vs.
            // numeric-add automatically based on the actual operand types.
            return "__cobalt_add__(" + emitExpr(b->lhs) + ", " + emitExpr(b->rhs) + ")";
        }
        return "(" + emitExpr(b->lhs) + " " + b->op + " " + emitExpr(b->rhs) + ")";
    }
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e))
        return emitExpr(idx->base) + "[" + emitExpr(idx->index) + "]";
    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) return pi->name + "++";
    if (auto pm = std::dynamic_pointer_cast<PostMinExpr>(e)) return pm->name + "--";
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        std::string out = "{";
        for (size_t i = 0; i < lit->items.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(lit->items[i]);
        }
        return out + "}";
    }
    if (auto fl = std::dynamic_pointer_cast<FracLit>(e)) {
        std::string out = "{";
        for (size_t i = 0; i < fl->items.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(fl->items[i]);
        }
        return out + "}";
    }
    if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) {
        std::string out = call->callee + "(";
        for (size_t i = 0; i < call->args.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(call->args[i]);
        }
        return out + ")";
    }
    if (auto c = std::dynamic_pointer_cast<ConcatExpr>(e)) {
        std::string out = "(";
        for (size_t i = 0; i < c->pieces.size(); i++) {
            if (i) out += " + ";
            out += emitExpr(c->pieces[i]);
        }
        return out + ")";
    }
    if (auto m = std::dynamic_pointer_cast<MemberExpr>(e)) {
        return emitExpr(m->object) + "." + m->member; // Assuming member access is valid in C++
    }
    if (auto mc = std::dynamic_pointer_cast<MethodCallExpr>(e)) {
        std::string out = emitExpr(mc->object);
        out += ".";
        out += mc->method;
        out += "(";

        for (size_t i = 0; i < mc->args.size(); i++)
        {
            if (i) out += ", ";
            out += emitExpr(mc->args[i]);
        }

        out += ")";
        return out;
    }
    throw std::runtime_error("Unknown expression.");
}

static void emitPrintStmt(const ExprPtr& value, bool newline, int depth, std::ofstream& out) {
    out << indent(depth) << "std::cout";
    if (value) {
        if (auto c = std::dynamic_pointer_cast<ConcatExpr>(value)) {
            out << " << " << emitConcatPieces(c);
        }
        else {
            out << " << " << emitExpr(value);
        }
    }
    if (newline) out << " << \"\\n\"";
    out << ";\n";
}
// println!("fmt {} {}", a, b) is parsed as a ConcatExpr whose first piece
// is the format-string literal and whose remaining pieces are the
// substitution arguments. std::print/std::println (<print>, C++23) are
// real functions that take the format string as their first argument and
// do their own {} substitution -- they don't have an operator<<, so this
// forwards the format string and args straight through as call arguments:
//   std::println("Name = {}\nAge = {}", Info.name, Info.age);
static void emitPrintMacStmt(const ExprPtr& value, bool newline, int depth, std::ofstream& out) {
    out << indent(depth) << (newline ? "std::println(" : "std::print(");
    if (value) {
        if (auto c = std::dynamic_pointer_cast<ConcatExpr>(value); c && !c->pieces.empty()) {
            out << emitExpr(c->pieces[0]);
            for (size_t i = 1; i < c->pieces.size(); i++) {
                out << ", " << emitExpr(c->pieces[i]);
            }
        }
        else {
            out << emitExpr(value);
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

static void emitStmt(const StmtPtr& stmt, int depth, std::ofstream& out);

static void emitBlock(const std::vector<StmtPtr>& body, int depth, std::ofstream& out) {
    for (const auto& s : body) emitStmt(s, depth, out);
}

static void emitStmt(const StmtPtr& stmt, int depth, std::ofstream& out) {
    if (auto v = std::dynamic_pointer_cast<VarDecl>(stmt)) {
        out << indent(depth);
        if (v->type == "List") {
            out << "std::vector<" << cppType(v->elemType) << "> " << v->name;
        }
        else if (v->type == "Fraction") {
            out << "__Fraction__<" << cppType(v->elemType) << ", " << cppType(v->secElemType) << "> " << v->name;
        }
        else if (v->arraySize >= 0) {
            out << cppType(v->type) << " " << v->name << "[" << v->arraySize << "]";
        }
        else {
            out << cppType(v->type) << " " << v->name;
        }
        out << " = " << emitExpr(v->init);
        out << ";\n";
        return;
    }
    if (auto f = std::dynamic_pointer_cast<CFDecl>(stmt)) {
        out << indent(depth) << emitCFDSignature(*f) << " {\n";
        emitBlock(f->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto a = std::dynamic_pointer_cast<AssignStmt>(stmt)) {
        out << indent(depth) << a->name << " = " << emitExpr(a->value) << ";\n";
        return;
    }
    if (auto ea = std::dynamic_pointer_cast<ExprAssignStmt>(stmt)) {
        out << indent(depth) << emitExpr(ea->target) << " = " << emitExpr(ea->value) << ";\n";
        return;
    }
    if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        out << indent(depth) << "return" << (r->value ? " " + emitExpr(r->value) : "") << ";\n";
        return;
    }
    if (auto p = std::dynamic_pointer_cast<PrintCode>(stmt)) {
        emitPrintStmt(p->value, p->newline, depth, out);
        return;
    }
    if (auto p = std::dynamic_pointer_cast<PrintMacCode>(stmt)) {
        emitPrintMacStmt(p->value, p->newline, depth, out);
        return;
    }
    if (auto in = std::dynamic_pointer_cast<ReadCode>(stmt)) {
        if (in->prompt) {
            out << indent(depth) << "std::cout << " << emitExpr(in->prompt) << ";\n";
        }
        out << indent(depth) << "std::cin >> " << emitExpr(in->target) << ";\n";
        return;
    }
    if (auto inStr = std::dynamic_pointer_cast<ReadLine>(stmt)) {
        if (inStr->prompt) {
            out << indent(depth) << "std::cout << " << emitExpr(inStr->prompt) << ";\n";
        }
        if (inStr->limit.empty()) {
            out << indent(depth) << "__cobalt_readln__(" << emitExpr(inStr->target) << ");\n";
        }
        else {
            // A char limit/delimiter only makes sense reading into text.
            out << indent(depth) << "std::getline(std::cin, " << emitExpr(inStr->target) << ", " << inStr->limit << ");\n";
        }
        return;
    }
    if (auto es = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        out << indent(depth) << emitExpr(es->expr) << ";\n";
        return;
    }
    if (auto i = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        out << indent(depth) << "if (" << emitExpr(i->condition) << ") {\n";
        emitBlock(i->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto e = std::dynamic_pointer_cast<ElifStmt>(stmt)) {
        out << indent(depth) << "else if (" << emitExpr(e->condition) << ") {\n";
        emitBlock(e->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto e = std::dynamic_pointer_cast<ElseStmt>(stmt)) {
        out << indent(depth) << "else {\n";
        emitBlock(e->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        out << indent(depth) << "while (" << emitExpr(w->condition) << ") {\n";
        emitBlock(w->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(stmt)) {
        out << indent(depth) << "for (int " << f->varName << " = " << emitExpr(f->from) << "; " <<
            f->varName << " <= " << emitExpr(f->to) << "; " << f->varName << "++) {\n";
        emitBlock(f->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto c = std::dynamic_pointer_cast<ContinueStmt>(stmt)) {
        emitContinueStmt(depth, out);
        return;
    }
    if (auto b = std::dynamic_pointer_cast<BreakStmt>(stmt)) {
        emitBreakStmt(depth, out);
        return;
    }
    if (auto b = std::dynamic_pointer_cast<ClearStmt>(stmt)) {
        emitClearStmt(depth, out);
        return;
    }
    if (auto cl = std::dynamic_pointer_cast<MethodCallExpr>(stmt)) {
        out << indent(depth);
        out << emitExpr(cl->object);   // Convert ExprPtr to string
        out << ".";
        out << cl->method;
        out << "(";

        for (size_t i = 0; i < cl->args.size(); i++)
        {
            if (i) out << ", ";
            out << emitExpr(cl->args[i]);
        }

        out << ");\n";
        return;
    }
    if (auto rp = std::dynamic_pointer_cast<RepeatCode>(stmt)) {
        out << indent(depth)
            << "for (int __value__ = 0; __value__ < "
            << emitExpr(rp->value)
            << "; __value__++) {\n";
        emitBlock(rp->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto fr = std::dynamic_pointer_cast<ForeverCode>(stmt)) {
        out << indent(depth) << "while (true) {\n";
        emitBlock(fr->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }
    if (auto lfn = std::dynamic_pointer_cast<LambFuncDecl>(stmt)) {
        out << indent(depth) << emitLambSignature(*lfn) << " {\n";
        emitBlock(lfn->body, depth + 1, out);
        out << indent(depth) << "};\n";
        return;
    }

    std::cerr << "codeGen error: no emitter for this statement type.\n";
    std::exit(EXIT_FAILURE);
}


static std::string emitSignature(const FunctionDecl& fn) {
    std::string out = cppType(fn.returnType) + " " + fn.name + "(";
    for (size_t i = 0; i < fn.params.size(); i++) {
        if (i) out += ", ";
        out += cppType(fn.params[i].type) + " " + fn.params[i].name;
    }
    return out += ")";
}

void codeGen(Program& program, std::string fileName, const std::string& inputFileDir) {
    std::string outcpp = fileName + ".cpp";
    std::ofstream file(outcpp);
    if (!file) {
        std::cerr << "Cannot create " << outcpp << ".\n";
        std::exit(EXIT_FAILURE);
    }

    file << "#include <iostream>\n";
    file << "#include <print>\n";
    file << "#include <string>\n";
    file << "#include <vector>\n";
    file << "#include <cstdint>\n";
    file << "#include <limits>\n";
    file << "#include <utility>\n";
    file << "#include <sstream>\n";
    file << "#include <type_traits>\n";
    file << "\n";
    file << "// `+` in Cobalt doubles as both numeric addition and string\n";
    file << "// concatenation (`height + \"cm\"`) -- plain C++ `+` doesn't compile for\n";
    file << "// e.g. int + const char*, so this picks string-concat vs. numeric-add\n";
    file << "// based on the actual operand types.\n";
    file << "template<typename __L__, typename __R__>\n";
    file << "auto __cobalt_add__(const __L__& l, const __R__& r) {\n";
    file << "    if constexpr (std::is_convertible_v<std::decay_t<__L__>, std::string> ||\n";
    file << "                  std::is_convertible_v<std::decay_t<__R__>, std::string>) {\n";
    file << "        std::ostringstream __oss__;\n";
    file << "        __oss__ << l << r;\n";
    file << "        return __oss__.str();\n";
    file << "    }\n";
    file << "    else {\n";
    file << "        return l + r;\n";
    file << "    }\n";
    file << "}\n";
    file << "\n";
    file << "// readln(...) reads a whole line (unlike read(...), which stops at the\n";
    file << "// first whitespace) -- but the target isn't always a std::string (e.g.\n";
    file << "// `readln(Info.age)` where age is int). std::getline only accepts a\n";
    file << "// std::string, so this reads the line as text once and then converts it\n";
    file << "// to whatever the target's real type is.\n";
    file << "template<typename __T__>\n";
    file << "void __cobalt_readln__(__T__& target) {\n";
    file << "    std::string __line__;\n";
    file << "    std::getline(std::cin, __line__);\n";
    file << "    std::istringstream __iss__(__line__);\n";
    file << "    __iss__ >> target;\n";
    file << "}\n";
    file << "inline void __cobalt_readln__(std::string& target) {\n";
    file << "    std::getline(std::cin, target);\n";
    file << "}\n";
    file << "\n";
    file << "// Backing type for Cobalt's frac<T> / frac<T1,T2> -- a simple\n";
    file << "// numerator/denominator pair that prints as \"num/denom\", and\n";
    file << "// converts implicitly to std::pair<A,B> for libraries (like a\n";
    file << "// math library) written against std::pair directly.\n";
    file << "template<typename __FracA__, typename __FracB__>\n";
    file << "struct __Fraction__ {\n";
    file << "    __FracA__ first;\n";
    file << "    __FracB__ second;\n";
    file << "    operator std::pair<__FracA__, __FracB__>() const { return {first, second}; }\n";
    file << "};\n";
    file << "template<typename __FracA__, typename __FracB__>\n";
    file << "std::ostream& operator<<(std::ostream& os, const __Fraction__<__FracA__, __FracB__>& f) {\n";
    file << "    return os << f.first << \"/\" << f.second;\n";
    file << "}\n";

    // Some libraries (e.g. chart, sdl3_win64) ship their own global
    // singleton instance (`__Table__ Table;` defined once in their .cpp,
    // `extern __Table__ Table;` declared in their header) rather than
    // relying on the auto-declared instance codeGen normally creates for an
    // object name it doesn't recognize (see the usedObjects loop below).
    // Scan each imported library's header(s) for that `extern` pattern so
    // we know which names are already provided -- otherwise codeGen would
    // also emit `__Table__ Table;` in the generated main file and the
    // linker would see two definitions of `Table`.
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

            // Bundle libraries can spread their globals across several
            // headers (Window.hpp, Draw.hpp, ...) beyond just the one
            // matching the import name, so scan every header in the bundle.
            std::error_code dirEc;
            for (const auto& entry : fs::directory_iterator(bundleDir, dirEc)) {
                if (dirEc) break;
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension();
                if (ext == ".hpp" || ext == ".h") scanFileForExternGlobals(entry.path());
            }
        }
        if (headerPath.empty()) {
            headerPath = findLibraryFile(imp.libName, ".hpp", inputFileDir);
        }
        if (!headerPath.empty() && bundleDir.empty()) {
            scanFileForExternGlobals(headerPath);
        }

        if (headerPath.empty()) {
            // Not found anywhere -- fall back to the old behavior and
            // let the C++ compiler report a clear "file not found".
            file << "#include \"" << imp.libName << ".hpp\"\n";
        }
        else {
            file << "#include \"" << fs::path(headerPath).generic_string() << "\"\n";
        }
    }
    file << "\n";

    for (const ClassDecl& cls : program.classes) {
        file << "class " << "__" << cls.name << "__ {\n";
        if (cls.pub) {
            file << "public:\n";
            emitBlock(cls.publicBody, 1, file);
        }
        if (cls.pvr) {
            file << "private:\n";
            emitBlock(cls.privateBody, 1, file);
        }
        file << "};\n";
    }

    for (const StructCode& str : program.struc) {
        file << "struct __" << str.name << "__ {\n";
        emitBlock(str.body, 1, file);
        file << "};\n";
    }

    for (const StructCode& str : program.struc) {
        file << "__" << str.name << "__ " << str.name << ";\n";
    }

    for (const ClassDecl& cls : program.classes) {
        file << "__" << cls.name << "__ " << cls.name << ";\n";
    }

    file << "\n";

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << ";\n";
    }
    file << "\n";
    for (auto& obj : program.usedObjects)
    {
        bool alreadyDeclared = false;
        for (const ClassDecl& cls : program.classes) {
            if (cls.name == obj) { alreadyDeclared = true; break; }
        }
        if (libraryProvidedGlobals.count(obj)) alreadyDeclared = true;
        if (!alreadyDeclared) {
            file << "__" << obj << "__ " << obj << ";\n";
        }
    }

    for (const CFuncDecl& cfnd : program.cfunctions) {
        file << emitCFSignature(cfnd) << " {\n";
        emitBlock(cfnd.body, 1, file);
        file << "}\n\n";
    }

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << " {\n";
        emitBlock(fn.body, 1, file);
        file << "}\n\n";
    }
}