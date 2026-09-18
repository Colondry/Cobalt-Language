#ifndef CODEGEN_VISITOR_HPP
#define CODEGEN_VISITOR_HPP

#include "ast.hpp"
#include "visitor.hpp"
#include "cg_helpers.hpp"
#include <string>
#include <vector>
#include <memory>

class CodeGenVisitor : public ExprVisitor, public StmtVisitor {
public:
    // ---- Context (saved/restored around recursive emitExpr calls) ----
    bool uns = false;
    std::string type = "";
    int depth = 1;

    // ---- Entry point (mirrors the old free function `emitExpr`) ----
    std::string emitExpr(const ExprPtr& e, bool uns_ = false, std::string type_ = "") {
        if (!e) return "";
        bool oldUns = uns;
        std::string oldType = type;
        uns = uns_;
        type = std::move(type_);
        std::string result = e->accept(*this);
        uns = oldUns;
        type = std::move(oldType);
        return result;
    }

    // ---- Statement entry point ----
    std::string emitStmt(const StmtPtr& s, int d) {
        if (!s) return "";
        int oldDepth = depth;
        depth = d;
        std::string result = s->accept(*this);
        depth = oldDepth;
        return result;
    }

    // ---- Helper: emit a whole block, joined with no separator ----
    std::string emitBlock(const std::vector<StmtPtr>& body, int d) {
        std::string out;
        for (const auto& s : body) out += emitStmt(s, d);
        return out;
    }

    // ===========================================================
    // Expression visitors — mirrors the old `emitExpr()` exactly.
    // ===========================================================

    std::string visit(NumberLit& n) override { return n.value; }
    std::string visit(StringLit& s) override { return s.value; }
    std::string visit(CharLit& c)   override { return c.value; }
    std::string visit(LnQuote& l)   override { return l.value; }

    std::string visit(NameExpr& n) override { return n.name; }

    std::string visit(PostIncExpr& p) override { return "(" + p.name + ")++"; }
    std::string visit(PostMinExpr& p) override { return "(" + p.name + ")--"; }

    std::string visit(UnaryExpr& u) override {
        if (u.op == "$") return "std::move(" + emitExpr(u.operand) + ")";
        if (u.op == "&") return "std::make_unique<" + type + ">(*(" + emitExpr(u.operand) + "))";
        if (u.op == "*") return "(*(" + emitExpr(u.operand) + "))";
        if (u.op == "-") return "(-(" + emitExpr(u.operand) + "))";
        if (u.op == "!") return "(!" + emitExpr(u.operand) + ")";
        return "/* unknown unary " + u.op + " */";
    }

    std::string visit(BinaryExpr& b) override {
        if (b.op == "+") {
            return "__cadd__(" + emitExpr(b.lhs) + ", " + emitExpr(b.rhs) + ")";
        }
        return "(" + emitExpr(b.lhs) + " " + b.op + " " + emitExpr(b.rhs) + ")";
    }

    std::string visit(IndexExpr& i) override {
        return "(" + emitExpr(i.base) + ")[" + emitExpr(i.index) + "]";
    }

    std::string visit(CallExpr& c) override {
        std::string out = c.callee + "(";
        for (size_t i = 0; i < c.args.size(); ++i) {
            if (i) out += ", ";
            out += "unwrap_val(" + emitExpr(c.args[i]) + ")";
        }
        return out + ")";
    }

    std::string visit(ListLit& l) override {
        std::string inner = "{";
        for (size_t i = 0; i < l.items.size(); ++i) {
            if (i) inner += ", ";
            inner += emitExpr(l.items[i]);
        }
        inner += "}";
        if (!uns) {
            std::string first = l.items.empty() ? "0" : emitExpr(l.items[0]);
            return "std::make_unique<std::vector<decltype(" + first + ")>>"
                   "(std::vector<" + type + ">" + inner + ")";
        }
        return inner;
    }

    std::string visit(FracLit& f) override {
        std::string inner = "{";
        for (size_t i = 0; i < f.items.size(); ++i) {
            if (i) inner += ", ";
            inner += emitExpr(f.items[i]);
        }
        inner += "}";
        return uns ? inner : "std::make_unique<frac>(" + inner + ")";
    }

    std::string visit(ConcatExpr& c) override {
        std::string out = "(";
        for (size_t i = 0; i < c.pieces.size(); ++i) {
            if (i) out += " + ";
            out += emitExpr(c.pieces[i]);
        }
        return out + ")";
    }

    std::string visit(MemberExpr& m) override {
        return emitExpr(m.object) + "." + m.member;
    }

    std::string visit(MethodMemberExpr& m) override {
        return emitExpr(m.object) + "::" + m.member;
    }

    std::string visit(MethodCallExpr& m) override {
        std::string out = emitExpr(m.object) + "." + m.method + "(";
        for (size_t i = 0; i < m.args.size(); ++i) {
            if (i) out += ", ";
            out += emitExpr(m.args[i]);
        }
        return out + ")";
    }

    std::string visit(NamespaceCallExpr& n) override {
        std::string out = emitExpr(n.object) + "::" + n.method + "(";
        for (size_t i = 0; i < n.args.size(); ++i) {
            if (i) out += ", ";
            out += emitExpr(n.args[i]);
        }
        return out + ")";
    }

    std::string visit(PointerExpr& p) override {
        // Fill in from whatever the old code did. Placeholder:
        return "(*(" + emitExpr(p.object) + "))";
    }

    // ===========================================================
    // Statement visitors — mirrors the old `emitStmt()` exactly.
    // ===========================================================

    std::string visit(VarDecl& v) override {
        std::string constOnPointee = (!v.uns && v.cptr) ? "const " : "";
        std::string constOnPtr     = (!v.uns && v.c)    ? "const " : "";
        std::string constOnVar     = (v.uns && v.c)     ? "const " : "";
        std::string pad            = indent(depth);

        if (v.type == "List") {
            std::string vecType = "std::vector<" + cppType(v.elemType) + ">";
            if (!v.uns)
                return pad + constOnPtr + "std::unique_ptr<" + constOnPointee + vecType + "> "
                       + v.name + " = "
                       + (v.init ? emitExpr(v.init, v.uns, cppType(v.elemType)) : "nullptr") + ";\n";
            return pad + constOnVar + vecType + " " + v.name + " = "
                   + (v.init ? emitExpr(v.init, v.uns, cppType(v.elemType)) : "{}") + ";\n";
        }
        if (v.type == "Fraction") [[unlikely]] {
            std::string fracType = "frac<" + cppType(v.elemType) + ", " + cppType(v.secElemType) + ">";
            if (!v.uns)
                return pad + constOnPtr + "std::unique_ptr<" + constOnPointee + fracType + "> "
                       + v.name + " = "
                       + (v.init ? emitExpr(v.init, v.uns, fracType) : "nullptr") + ";\n";
            return pad + constOnVar + fracType + " " + v.name + " = "
                   + (v.init ? emitExpr(v.init, v.uns, fracType) : "{}") + ";\n";
        }
        if (v.type == "auto") {
            std::string init = emitExpr(v.init, v.uns);
            if (!v.uns)
                return pad + "auto " + v.name + " = std::make_unique<"
                       + constOnPointee + "std::decay_t<decltype(" + init + ")>>(" + init + ");\n";
            return pad + "auto " + v.name + " = " + init + ";\n";
        }
        std::string cppT = cppType(v.type);
        if (!v.uns) {
            std::string result = pad + constOnPtr + "std::unique_ptr<"
                                 + constOnPointee + cppT + "> " + v.name;
            if (v.init)
                result += " = std::make_unique<" + constOnPointee + cppT + ">("
                          + emitExpr(v.init, v.uns, cppT) + ")";
            return result + ";\n";
        }
        std::string result = pad + constOnVar + cppT + " " + v.name;
        if (v.init) result += " = (" + emitExpr(v.init, v.uns, cppT) + ")";
        return result + ";\n";
    }

    std::string visit(TypeDecl& t) override {
        std::string line;
        if (t.type == "List")
            line = "using " + t.name + " = std::vector<" + cppType(t.elemType) + ">;";
        else if (t.type == "Fraction")
            line = "using " + t.name + " = frac<" + cppType(t.elemType) + ", " + cppType(t.secElemType) + ">;";
        else if (t.arraySize >= 0)
            line = "using " + t.name + " = " + cppType(t.type) + "[" + std::to_string(t.arraySize) + "];";
        else
            line = "using " + t.name + " = " + cppType(t.type) + ";";
        return indent(depth) + line + "\n";
    }

    std::string visit(AssignStmt& a) override {
        return indent(depth) + a.name + " = " + emitExpr(a.value) + ";\n";
    }
    std::string visit(ExprAssignStmt& a) override {
        return indent(depth) + emitExpr(a.target) + " = " + emitExpr(a.value) + ";\n";
    }
    std::string visit(ReturnStmt& r) override {
        return indent(depth) + "return" + (r.value ? " " + emitExpr(r.value) : "") + ";\n";
    }
    std::string visit(ExprStmt& e) override {
        return indent(depth) + emitExpr(e.expr) + ";\n";
    }

    std::string visit(PrintCode& p) override {
        std::string out = indent(depth) + "std::cout";
        if (p.value) out += " << " + emitExpr(p.value);
        if (p.newline) out += " << \"\\n\"";
        return out + ";\n";
    }

    std::string visit(PrintMacCode& p) override {
        std::string out = indent(depth) + (p.newline ? "println_c(" : "print_c(");
        if (p.value) {
            if (auto c = std::dynamic_pointer_cast<ConcatExpr>(p.value); c && !c->pieces.empty()) {
                out += emitExpr(c->pieces[0]);
                for (size_t i = 1; i < c->pieces.size(); ++i)
                    out += ", " + emitExpr(c->pieces[i]);
            } else {
                out += emitExpr(p.value);
            }
        }
        return out + ");\n";
    }

    std::string visit(ReadCode& r) override {
        std::string out;
        if (r.prompt) out += indent(depth) + "std::cout << " + emitExpr(r.prompt) + ";\n";
        out += indent(depth) + "std::cin >> (*" + emitExpr(r.target) + ");\n";
        return out;
    }

    std::string visit(ReadLine& r) override {
        std::string out;
        if (r.prompt) out += indent(depth) + "std::cout << " + emitExpr(r.prompt) + ";\n";
        out += indent(depth) + "readln(" + emitExpr(r.prompt ? r.prompt : nullptr)
               + ", " + emitExpr(r.target);
        if (!r.limit.empty()) out += ", '" + r.limit + "'";
        out += ");\n";
        return out;
    }

    std::string visit(BreakStmt&)    override { return indent(depth) + "break;\n"; }
    std::string visit(ContinueStmt&) override { return indent(depth) + "continue;\n"; }
    std::string visit(ClearStmt&)    override {
#ifdef _WIN32
        return indent(depth) + "system(\"cls\");\n";
#else
        return indent(depth) + "std::cout << \"\\033[2J\\033[H\";\n"
             + indent(depth) + "std::cout.flush();\n";
#endif
    }

    std::string visit(RepeatCode& r) override {
        std::string out = indent(depth) + "for (int __value__ = 0; __value__ < "
                        + emitExpr(r.value) + "; __value__++) {\n";
        out += emitBlock(r.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(ForeverCode& f) override {
        std::string out = indent(depth) + "while (true) {\n";
        out += emitBlock(f.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(IfStmt& i) override {
        std::string out = indent(depth) + "if (" + emitExpr(i.condition) + ") "
                        + (i.lik ? "[[likely]]" : "") + (i.unl ? "[[unlikely]]" : "") + " {\n";
        out += emitBlock(i.body, depth + 1);
        out += indent(depth) + "}\n";
        if (i.iselif) {
            out += indent(depth) + "else if (" + emitExpr(i.elifCond) + ") "
                 + (i.eilik ? "[[likely]]" : "") + (i.eiunl ? "[[unlikely]]" : "") + " {\n";
            out += emitBlock(i.elifbody, depth + 1);
            out += indent(depth) + "}\n";
        }
        if (i.iselse) {
            out += indent(depth) + "else "
                 + (i.elik ? "[[likely]]" : "") + (i.eunl ? "[[unlikely]]" : "") + " {\n";
            out += emitBlock(i.elsebody, depth + 1);
            out += indent(depth) + "}\n";
        }
        return out;
    }

    std::string visit(ElifStmt& e) override {
        std::string out = indent(depth) + "else if (" + emitExpr(e.condition) + ") "
                        + (e.lik ? "[[likely]]" : "") + (e.unl ? "[[unlikely]]" : "") + " {\n";
        out += emitBlock(e.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(ElseStmt& e) override {
        std::string out = indent(depth) + "else "
                        + (e.lik ? "[[likely]]" : "") + (e.unl ? "[[unlikely]]" : "") + " {\n";
        out += emitBlock(e.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(WhileStmt& w) override {
        std::string out = indent(depth) + "while (" + emitExpr(w.condition) + ") {\n";
        out += emitBlock(w.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(DoStmt& d) override {
        std::string out = indent(depth) + "for (int cobalt_do_repeat = "
                        + emitExpr(d.start) + "; cobalt_do_repeat < "
                        + emitExpr(d.end) + "; ++cobalt_do_repeat) {\n";
        out += emitBlock(d.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(ForRangeStmt& f) override {
        if (auto call = std::dynamic_pointer_cast<CallExpr>(f.condition)) {
            if (call->callee == "range" && call->args.size() == 2) {
                std::string start = emitExpr(call->args[0]);
                std::string end   = emitExpr(call->args[1]);
                std::string out = indent(depth) + "for (int64_t " + f.varName + " = " + start
                                + "; " + f.varName + " < " + end
                                + "; ++" + f.varName + ") {\n";
                out += emitBlock(f.body, depth + 1);
                out += indent(depth) + "}\n";
                return out;
            }
        }
        std::string out = indent(depth) + "for (auto&& " + f.varName + " : "
                        + emitExpr(f.condition) + ") {\n";
        out += emitBlock(f.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(TryExcept& t) override {
        std::string out;
        if (t.hasExcept && !t.nec)
            out += indent(depth) + "cobalt__try_status__ = 0;\n";
        out += indent(depth) + "try {\n";
        out += emitBlock(t.tryBody, depth + 1);
        out += indent(depth) + "}\n";
        if (!t.hasExcept) {
            out += indent(depth) + "catch (...) {}\n";
        } else if (!t.nec) {
            out += indent(depth) + "catch (const std::exception& e) {\n";
            out += indent(depth + 1) + "cobalt__try_status__ = 1;\n";
            out += indent(depth) + "} catch (...) {\n";
            out += indent(depth + 1) + "cobalt__try_status__ = 1;\n";
            out += indent(depth) + "}\n";
            out += indent(depth) + "if (" + emitExpr(t.exceptCond) + ") {\n";
            out += emitBlock(t.exceptBody, depth + 1);
            out += indent(depth) + "}\n";
        } else {
            out += indent(depth) + "catch (const std::exception& e) {\n";
            out += emitBlock(t.exceptBody, depth + 1);
            out += indent(depth) + "} catch (...) {\n";
            out += emitBlock(t.exceptBody, depth + 1);
            out += indent(depth) + "}\n";
        }
        return out;
    }

    // --- Function declarations (mirror the old free emitters) ---

    std::string visit(CFDecl& f) override {
        std::string sig = cppType(f.returnType) + " " + f.name + "(";
        for (size_t i = 0; i < f.params.size(); ++i) {
            if (i) sig += ", ";
            sig += cppType(f.params[i].type) + " " + f.params[i].name;
        }
        sig += ")";
        std::string out = indent(depth) + sig + " {\n";
        out += emitBlock(f.body, depth + 1);
        out += indent(depth) + "}\n";
        return out;
    }

    std::string visit(CFuncDecl& f) override {
        std::string sig = cppType(f.returnType) + " " + f.name + "(";
        for (size_t i = 0; i < f.params.size(); ++i) {
            if (i) sig += ", ";
            sig += cppType(f.params[i].type) + " " + f.params[i].name;
        }
        sig += ")";
        std::string out = indent(depth) + sig + " {\n";
        out += emitBlock(f.body, depth + 1);
        out += indent(depth) + "}\n\n";
        return out;
    }

    std::string visit(LambFuncDecl& f) override {
        std::string sig = "auto " + f.name + " = [](";
        for (size_t i = 0; i < f.params.size(); ++i) {
            if (i) sig += ", ";
            sig += cppType(f.params[i].type) + " " + f.params[i].name;
        }
        sig += ") -> " + cppType(f.returnType);
        std::string out = indent(depth) + sig + " {\n";
        out += emitBlock(f.body, depth + 1);
        out += indent(depth) + "};\n";
        return out;
    }
};

#endif