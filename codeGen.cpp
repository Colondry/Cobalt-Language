#include "codeGen.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

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

static std::string emitExpr(const ExprPtr& e) {
    if (auto n = std::dynamic_pointer_cast<NumberLit>(e)) return n->value;
    if (auto s = std::dynamic_pointer_cast<StringLit>(e)) return s->value;
    if (auto id = std::dynamic_pointer_cast<NameExpr>(e)) return id->name;
    if (auto u = std::dynamic_pointer_cast<UnaryExpr>(e)) return "(-" + emitExpr(u->operand) + ")";
    if (auto b = std::dynamic_pointer_cast<BinaryExpr>(e))
        return "(" + emitExpr(b->lhs) + " " + b->op + " " + emitExpr(b->rhs) + ")";
    if (auto idx = std::dynamic_pointer_cast<IndexExpr>(e))
        return emitExpr(idx->base) + "[" + emitExpr(idx->index) + "]";
    if (auto pi = std::dynamic_pointer_cast<PostIncExpr>(e)) return pi->name + "++";
    if (auto lit = std::dynamic_pointer_cast<ListLit>(e)) {
        std::string out = "{";
        for (size_t i = 0; i < lit->items.size(); i++) {
            if (i) out += ", ";
            out += emitExpr(lit->items[i]);
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
    return "/* unknown expr */";
}

static void emitPrintStmt(const ExprPtr& value, bool newline, int depth, std::string& out) {
    out += indent(depth) + "std::cout";
    if (value) {
        if (auto c = std::dynamic_pointer_cast<ConcatExpr>(value)) {
            out += " << " + emitConcatPieces(c);
        } else {
            out += " << " + emitExpr(value);
        }
    }
    if (newline) out += " << \"\\n\"";
    out += ";\n";
}

static void emitStmt(const StmtPtr& stmt, int depth, std::string& out);

static void emitBlock(const std::vector<StmtPtr>& body, int depth, std::string& out) {
    for (const auto& s : body) emitStmt(s, depth, out);
}

static void emitStmt(const StmtPtr& stmt, int depth, std::string& out) {
    if (auto v = std::dynamic_pointer_cast<VarDecl>(stmt)) {
        out += indent(depth);
        if (v->type == "List") {
            out += "std::vector<" + cppType(v->elemType) + "> " + v->name;
        } else if (v->arraySize >= 0) {
            out += cppType(v->type) + " " + v->name + "[" + std::to_string(v->arraySize) + "]";
        } else {
            out += cppType(v->type) + " " + v->name;
        }
        if (v->init) out += " = " + emitExpr(v->init);
        out += ";\n";
        return;
    }
    if (auto a = std::dynamic_pointer_cast<AssignStmt>(stmt)) {
        out += indent(depth) + a->name + " = " + emitExpr(a->value) + ";\n";
        return;
    }
    if (auto r = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        out += indent(depth) + "return" + (r->value ? " " + emitExpr(r->value) : "") + ";\n";
        return;
    }
    if (auto p = std::dynamic_pointer_cast<PrintCode>(stmt)) {
        emitPrintStmt(p->value, p->newline, depth, out);
        return;
    }
    if (auto in = std::dynamic_pointer_cast<InputCode>(stmt)) {
        if (in->prompt) {
            out += indent(depth) + "std::cout << " + emitExpr(in->prompt) + ";\n";
        }
        out += indent(depth) + "std::cin >> " + in->varName + ";\n";
        return;
    }
    if (auto es = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        out += indent(depth) + emitExpr(es->expr) + ";\n";
        return;
    }
    if (auto i = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        out += indent(depth) + "if (" + emitExpr(i->condition) + ") {\n";
        emitBlock(i->body, depth + 1, out);
        out += indent(depth) + "}\n";
        return;
    }
    if (auto w = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        out += indent(depth) + "while (" + emitExpr(w->condition) + ") {\n";
        emitBlock(w->body, depth + 1, out);
        out += indent(depth) + "}\n";
        return;
    }
    if (auto f = std::dynamic_pointer_cast<ForRangeStmt>(stmt)) {
        out += indent(depth) + "for (int " + f->varName + " = " + emitExpr(f->from) + "; " +
               f->varName + " <= " + emitExpr(f->to) + "; " + f->varName + "++) {\n";
        emitBlock(f->body, depth + 1, out);
        out += indent(depth) + "}\n";
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
    return out + ")";
}

void codeGen(Program& program, std::string fileName) {
    std::string outcpp = fileName + ".cpp";
    std::ofstream file(outcpp);
    if (!file) {
        std::cerr << "Cannot create " << outcpp << ".\n";
        std::exit(EXIT_FAILURE);
    }

    file << "#include <iostream>\n";
    file << "#include <string>\n";
    file << "#include <vector>\n";
    file << "#include <cstdint>\n";

    for (const LibImport& imp : program.imports) {
        file << "#include \"" << imp.libName << ".hpp\"\n";
    }
    file << "\n";

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << ";\n";
    }
    file << "\n";

    for (const FunctionDecl& fn : program.functions) {
        file << emitSignature(fn) << " {\n";
        std::string body;
        emitBlock(fn.body, 1, body);
        file << body;
        file << "}\n\n";
    }
}