#include "codeGen.hpp"
#include "flib.hpp"
#include "Deadcode.hpp"
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

static std::string cppType(const std::string& t) {
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
    if (auto lq = std::dynamic_pointer_cast<LnQuote>(e)) return lq->value;
    if (auto c = std::dynamic_pointer_cast<CharLit>(e)) return c->value;
    if (auto id = std::dynamic_pointer_cast<NameExpr>(e)) return id->name;

    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) {
        if (u->op == "$") {
            // Transfers ownership; source unique_ptr becomes nullptr
            return "std::move(" + emitExpr(u->operand) + ")";
        }
        if (u->op == "&") {
            // Borrows underlying raw pointer; safely dereferenced via .get()
            return emitExpr(u->operand) + ".get()";
        }
        if (u->op == "-") {
            return "(-" + emitExpr(u->operand) + ")";
        }
        if (u->op == "!") {
            return "(!" + emitExpr(u->operand) + ")";
        }
    }

    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e)) {
        if (b->op == "+") {
            return "__cadd__(" + emitExpr(b->lhs) + ", " + emitExpr(b->rhs) + ")";
        }
        return "(" + emitExpr(b->lhs) + " " + b->op + " " + emitExpr(b->rhs) + ")";
    }

    // Dereference smart pointer before indexing
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e))
        return "(*" + emitExpr(idx->base) + ")[" + emitExpr(idx->index) + "]";

    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) return "(*" + pi->name + ")++";
    if (auto pm = std::dynamic_pointer_cast<PostMinExpr>(e)) return "(*" + pm->name + ")--";

    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        std::string inner = "{";
        for (size_t i = 0; i < lit->items.size(); i++) {
            if (i) inner += ", ";
            inner += emitExpr(lit->items[i]);
        }
        inner += "}";
        return "std::make_unique<std::vector<decltype(" + 
               (lit->items.empty() ? "0" : emitExpr(lit->items[0])) + ")>>(" + inner + ")";
    }

    if (auto fl = std::dynamic_pointer_cast<FracLit>(e)) {
        std::string inner = "{";
        for (size_t i = 0; i < fl->items.size(); i++) {
            if (i) inner += ", ";
            inner += emitExpr(fl->items[i]);
        }
        inner += "}";
        return "std::make_unique<frac>(" + inner + ")";
    }

    if (auto call = std::dynamic_pointer_cast<CallExpr>(e)) {
        std::string out = call->callee + "(";
        for (size_t i = 0; i < call->args.size(); i++) {
            if (i) out += ", ";
            // Parameters are plain T while locals are std::unique_ptr<T>; unwrap so the
            // argument's runtime type actually matches what the callee expects.
            out += "unwrap_val(" + emitExpr(call->args[i]) + ")";
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
        std::string obj = emitExpr(m->object);
        return obj + "." + m->member;
    }

    if (auto mc = std::dynamic_pointer_cast<MethodCallExpr>(e)) {
        std::string out = emitExpr(mc->object) + "." + mc->method + "(";
        for (size_t i = 0; i < mc->args.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(mc->args[i]);
        }
        return out + ")";
    }

    if (auto mm = std::dynamic_pointer_cast<NamespaceCallExpr>(e)) {
        std::string out = emitExpr(mm->object) + "::" + mm->method + "(";
        for (size_t i = 0; i < mm->args.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(mm->args[i]);
        }
        return out + ")";
    }

    if (auto mem = std::dynamic_pointer_cast<MethodMemberExpr>(e)) {
        return emitExpr(mem->object) + "::" + mem->member;
    }

    throw std::runtime_error("Unknown expression.");
}

// Helper function to emit unique_ptr wrapped initializers
static std::string emitInitExpr(const std::string& typeStr, const ExprPtr& initExpr) {
    if (!initExpr) return "nullptr";

    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(initExpr)) {
        if (u->op == "$") return emitExpr(initExpr);
    }

    std::string val = emitExpr(initExpr);
    return "std::make_unique<" + typeStr + ">(" + val + ")";
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
static void emitPrintMacStmt(const ExprPtr& value, bool newline, int depth, std::ofstream& out) {
    out << indent(depth) << (newline ? "println_c(" : "print_c(");
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
            std::string vecType = "std::vector<" + cppType(v->elemType) + ">";
            out << "std::unique_ptr<" << vecType << "> " << v->name;
            out << " = " << (v->init ? emitExpr(v->init) : "nullptr") << ";\n";
        }
        else if (v->type == "Fraction") {
            std::string fracType = "frac<" + cppType(v->elemType) + ", " + cppType(v->secElemType) + ">";
            out << "std::unique_ptr<" << fracType << "> " << v->name;
            out << " = " << (v->init ? emitExpr(v->init) : "nullptr") << ";\n";
        }
        else {
            std::string targetType = cppType(v->type);
            out << "std::unique_ptr<" << targetType << "> " << v->name;
            out << " = " << emitInitExpr(targetType, v->init) << ";\n";
        }
        return;
    }

    // Dereference target for input streams
    if (auto in = std::dynamic_pointer_cast<ReadCode>(stmt)) {
        if (in->prompt) {
            out << indent(depth) << "std::cout << " << emitExpr(in->prompt) << ";\n";
        }
        out << indent(depth) << "std::cin >> (*" << emitExpr(in->target) << ");\n";
        return;
    }

    // Dereference container for range-based loops
    if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(stmt)) {
        if (auto call = std::dynamic_pointer_cast<CallExpr>(f->condition)) {
            if (call->callee == "range" && call->args.size() == 2) {
                std::string start = emitExpr(call->args[0]);
                std::string end = emitExpr(call->args[1]);

                out << indent(depth) << "for (int " << f->varName << " = " << start 
                    << "; " << f->varName << " < " << end << "; " << f->varName << "++) {\n";
                emitBlock(f->body, depth + 1, out);
                out << indent(depth) << "}\n";
                return;
            }
        }

        // Dereference smart container pointer (*expr) for C++ range iteration
        out << indent(depth) << "for (auto&& " << f->varName << " : *(" << emitExpr(f->condition) << ")) {\n";
        emitBlock(f->body, depth + 1, out);
        out << indent(depth) << "}\n";
        return;
    }

    /* PRINT DEPRECEATED
    if (auto p = std::dynamic_pointer_cast<PrintCode>(stmt)) {
        out << indent(depth) << "std::cout";
        if (p->value) {
            out << " << (*" << emitExpr(p->value) << ")";
        }
        if (p->newline) out << " << \"\\n\"";
        out << ";\n";
        return;
    }
   */

    if (auto t = std::dynamic_pointer_cast<TypeDecl>(stmt)) {
        out << indent(depth) << emitTypeDeclLine(*t);
        return;
    }
    if (auto f = std::dynamic_pointer_cast<CFDecl>(stmt)) {
        out << indent(depth) << emitCFDSignature(*f) << " {\n";
        emitBlock(pruneAndReport(f->body, "method '" + f->name + "'"), depth + 1, out);
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
    if (auto p = std::dynamic_pointer_cast<PrintMacCode>(stmt)) {
        emitPrintMacStmt(p->value, p->newline, depth, out);
        return;
    }
    if (auto inStr = std::dynamic_pointer_cast<ReadLine>(stmt)) {
        out << indent(depth);
        out << "readln(" << emitExpr(inStr->prompt) << ", " << emitExpr(inStr->target);
        if (!inStr->limit.empty()) {
            out << ", '" << inStr->limit << "'";
        }
        out << ");\n";
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
    if (auto te = std::dynamic_pointer_cast<TryExcept>(stmt)) {
        // Reset status flag before execution so prior errors don't carry over
        if (te->hasExcept && !te->nec) {
            out << indent(depth) << "cobalt__try_status__ = 0;\n";
        }

        // Emit Try Body
        out << indent(depth) << "try {\n";
        emitBlock(te->tryBody, depth + 1, out);
        out << indent(depth) << "}\n";

        if (!te->hasExcept) {
            // Bare 'try' with no 'except' clause at all -- just swallow the exception.
            out << indent(depth) << "catch (...) {}\n";
        }
        // Emit Exception Handling
        else if (!te->nec) {
            // Conditional Status Handling
            out << indent(depth) << "catch (const std::exception& e) {\n";
            out << indent(depth + 1) << "cobalt__try_status__ = 1;\n";
            out << indent(depth) << "} catch (...) {\n";
            out << indent(depth + 1) << "cobalt__try_status__ = 1;\n";
            out << indent(depth) << "}\n";

            // Evaluate condition after trapped execution
            out << indent(depth) << "if (" << emitExpr(te->exceptCond) << ") {\n";
            emitBlock(te->exceptBody, depth + 1, out);
            out << indent(depth) << "}\n";
        } 
        else {
            // Standard Catch-All
            out << indent(depth) << "catch (const std::exception& e) {\n";
            emitBlock(te->exceptBody, depth + 1, out);
            out << indent(depth) << "} catch (...) {\n";
            emitBlock(te->exceptBody, depth + 1, out);
            out << indent(depth) << "}\n";
        }
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
        out << emitExpr(cl->object);
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
    if (auto nc = std::dynamic_pointer_cast<NamespaceCallExpr>(stmt)) {
        out << indent(depth);
        out << emitExpr(nc->object);
        out << "::";
        out << nc->method;
        out << "(";

        for (size_t i = 0; i < nc->args.size(); i++)
        {
            if (i) out << ", ";
            out << emitExpr(nc->args[i]);
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
        emitBlock(pruneAndReport(lfn->body, "lambda '" + lfn->name + "'"), depth + 1, out);
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
    file << "#include <vector>\n";
    file << "#include <cstdint>\n";
    file << "#include <stdfloat>\n\n";
    file << "#include <utility>\n";
    file << "#include <memory>\n";
    file << "inline void syncw_stdio(bool s) {\n";
    file << "   std::ios_base::sync_with_stdio(s);\n";
    file << "}\n";
    file << "inline thread_local int cobalt__try_status__ = 0;\n";
    file << "inline int TryStatus() { return cobalt__try_status__; }";

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

    for (const TypeDecl& td : program.typedefs) {
        file << emitTypeDeclLine(td);
    }
    for (const ModuleDecl& module : program.modules) {
        file << "namespace " << module.name << " {\n";
        emitBlock(module.body, 1, file);
        file << "}\n";
    }

    for (const ClassDecl& cls : program.classes) {
        file << "class " << cls.name << " {\n";
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
        file << "struct " << str.name << " {\n";
        emitBlock(str.body, 1, file);
        file << "};\n";
    }

    for (const ClassDecl& cls : program.classes) {
        file << cls.name << " " << cls.name << ";\n";
    }

    for (const AutoUse& au : program.autouses) {
        if (au.mode == 0) {
            file << au.libName << " " << au.libName << ";\n";
        } else {
            file << "using namespace " << au.libName << ";\n";
        }
    }

    file << "\n";

    for (const Use& u : program.uses) {
        if (u.mode == 0) {
            file << u.first << " " << u.second << ";\n";
        } else {
            file << "namespace " << u.first << " = " << u.second << ";\n";
        }
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
            file << obj << " " << obj << ";\n";
        }
    }

    for (const CFuncDecl& cfnd : program.cfunctions) {
        file << emitCFSignature(cfnd) << " {\n";
        emitBlock(pruneAndReport(cfnd.body, "function '" + cfnd.name + "'"), 1, file);
        file << "}\n\n";
    }

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << " {\n";
        if (fn.name == "main") {
            file << indent(1) << "syncw_stdio(false);\n";
        }
        emitBlock(pruneAndReport(fn.body, "function '" + fn.name + "'"), 1, file);
        file << "}\n\n";
    }
}