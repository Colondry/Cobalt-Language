#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <cstdint>
#include "visitor.hpp"

// ---------- Expressions ----------

class Param { public: std::string type; std::string name; };

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::string accept(ExprVisitor& v) = 0;
};
using ExprPtr = std::shared_ptr<Expr>;

class NumberLit : public Expr { 
public:
    std::string value; 
    NumberLit(std::string v) : value(std::move(v)) {}
    std::string accept(ExprVisitor& v) override;
};
class StringLit : public Expr {
 public: 
    std::string value; 
    StringLit(std::string v) : value(std::move(v)) {}
    std::string accept(ExprVisitor& v) override;
}; // includes quotes
class CharLit : public Expr {
     public: std::string value; 
     CharLit(std::string v) : value(std::move(v)) {}
     std::string accept(ExprVisitor& v) override;
}; // includes single quotes
class LnQuote : public Expr { 
    public: std::string value;
    LnQuote(std::string v) : value(std::move(v)) {}
    std::string accept(ExprVisitor& v) override;
}; // includes %"
class NameExpr : public Expr { 
    public: std::string name; 
    NameExpr(std::string n) : name(std::move(n)) {}
    std::string accept(ExprVisitor& v) override;
};
class ListLit : public Expr { 
    public: std::vector<ExprPtr> items; double index; 
    ListLit(std::vector<ExprPtr> i, double ind) : items(std::move(i)), index(std::move(ind)) {}
    std::string accept(ExprVisitor& v) override;
};
class FracLit : public Expr { public: std::vector<ExprPtr> items; double index; 
    FracLit(std::vector<ExprPtr> i, double ind) : items(std::move(i)), index(std::move(ind)) {}
    std::string accept(ExprVisitor& v) override;
};
class IndexExpr : public Expr { public: ExprPtr base; ExprPtr index; 
    IndexExpr(ExprPtr b, ExprPtr i) : base(std::move(b)), index(std::move(i)) {}
    std::string accept(ExprVisitor& v) override;
};
class CallExpr : public Expr { public: std::string callee; std::vector<ExprPtr> args; 
    CallExpr(std::string c, std::vector<ExprPtr> a) : callee(std::move(c)), args(std::move(a)) {}
    std::string accept(ExprVisitor& v) override;
};
class PostIncExpr : public Expr { public: std::string name; 
    PostIncExpr(std::string n) : name(std::move(n)) {}
    std::string accept(ExprVisitor& v) override;
};
class PostMinExpr : public Expr { public: std::string name; 
    PostMinExpr(std::string n) : name(std::move(n)) {}
    std::string accept(ExprVisitor& v) override;
};
class UnaryExpr : public Expr { public: std::string op; ExprPtr operand; 
    UnaryExpr(std::string o, ExprPtr opnd) : op(std::move(o)), operand(std::move(opnd)) {}
    std::string accept(ExprVisitor& v) override;
};
class BinaryExpr : public Expr { public: std::string op; ExprPtr lhs; ExprPtr rhs; 
    BinaryExpr(std::string o, ExprPtr l, ExprPtr r) : op(std::move(o)), lhs(std::move(l)), rhs(std::move(r)) {}
    std::string accept(ExprVisitor& v) override;
};

class ConcatExpr : public Expr {
public:
    std::vector<ExprPtr> pieces;
    ConcatExpr(std::vector<ExprPtr> p) : pieces(std::move(p)) {}
    std::string accept(ExprVisitor& v) override;
};

// ---------- Statements ----------

class Stmt { public: virtual ~Stmt() = default; 
    virtual std::string accept(class StmtVisitor& v) = 0; 
};
using StmtPtr = std::shared_ptr<Stmt>;

class VarDecl : public Stmt {
public:
    std::string type;         // int, string, float, double, byte, char, List
    std::string elemType;     // element type when type == "List"
    std::string secElemType;  // element type when type == "frac"
    std::string name;         // name of the variable
    int arraySize = -1;       // >=0 for "char c[20]"
    ExprPtr init;             // never null -- every declaration requires an initializer (memory safety)
    bool isNull = false;      // always false; kept for structural compatibility
    bool c, cptr; 
    bool uns = true;

    VarDecl(std::string t, std::string et, std::string set, std::string n, int as, ExprPtr i, bool c_, bool cptr_, bool uns_) :
        type(std::move(t)), elemType(std::move(et)), secElemType(std::move(set)), name(std::move(n)), arraySize(as), init(std::move(i)), c(c_), cptr(cptr_), uns(uns_) {}
    std::string accept(class StmtVisitor& v) override;
};
class TypeDecl : public Stmt {
public:
    std::string type;         // int, string, float, double, byte, char, List
    std::string elemType;     // element type when type == "List"
    std::string secElemType;  // element type when type == "frac"
    std::string name;         // name of the variable
    int arraySize = -1;       // >=0 for "char c[20]"

    TypeDecl(std::string t, std::string et, std::string set, std::string n, int as) :
        type(std::move(t)), elemType(std::move(et)), secElemType(std::move(set)), name(std::move(n)), arraySize(as) {}
    std::string accept(class StmtVisitor& v) override;
};

struct MemberExpr : Expr
{
    ExprPtr object;
    std::string member;

    MemberExpr(ExprPtr obj, std::string mem) : object(std::move(obj)), member(std::move(mem)) {}
    std::string accept(ExprVisitor& v) override;
};
struct MethodMemberExpr : Expr
{
    ExprPtr object;
    std::string member;
    MethodMemberExpr(ExprPtr obj, std::string mem) : object(std::move(obj)), member(std::move(mem)) {}
    std::string accept(ExprVisitor& v) override;
};
struct PointerExpr : Expr {
    ExprPtr object;
    std::string member;

    PointerExpr(ExprPtr obj, std::string mem) : object(std::move(obj)), member(std::move(mem)) {}
    std::string accept(ExprVisitor& v) override;
};

struct MethodCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;

    MethodCallExpr(ExprPtr obj, std::string m, std::vector<ExprPtr> a) : object(std::move(obj)), method(std::move(m)), args(std::move(a)) {}
    std::string accept(ExprVisitor& v) override;
};
struct NamespaceCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;

    NamespaceCallExpr(ExprPtr obj, std::string m, std::vector<ExprPtr> a) : object(std::move(obj)), method(std::move(m)), args(std::move(a)) {}
    std::string accept(ExprVisitor& v) override;
};

class AssignStmt : public Stmt { public: std::string name; ExprPtr value; 
    AssignStmt(std::string n, ExprPtr v) : name(std::move(n)), value(std::move(v)) {}
    std::string accept(class StmtVisitor& v) override;
};
class ExprAssignStmt : public Stmt { public: ExprPtr target; ExprPtr value; 
    ExprAssignStmt(ExprPtr t, ExprPtr v) : target(std::move(t)), value(std::move(v)) {}
    std::string accept(class StmtVisitor& v) override;
};
class ReturnStmt : public Stmt { public: ExprPtr value; 
    ReturnStmt(ExprPtr v) : value(std::move(v)) {}
    std::string accept(class StmtVisitor& v) override;
}; // value may be null
class ExprStmt : public Stmt { public: ExprPtr expr; 
    ExprStmt(ExprPtr e) : expr(std::move(e)) {}
    std::string accept(class StmtVisitor& v) override;
};

class PrintCode : public Stmt { public: bool newline = false; ExprPtr value; int toRight = 1; 
    PrintCode(bool nl, ExprPtr v, int toRight) : newline(nl), value(std::move(v)), toRight(toRight) {}
    std::string accept(class StmtVisitor& v) override;
};
class PrintMacCode : public Stmt { public: bool newline = false; ExprPtr value;
    PrintMacCode(bool nl, ExprPtr v) : newline(nl), value(std::move(v)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ReadCode : public Stmt { public: ExprPtr prompt; ExprPtr target; 
    ReadCode(ExprPtr p, ExprPtr t) : prompt(std::move(p)), target(std::move(t)) {}
    std::string accept(class StmtVisitor& v) override;
};
class ReadLine : public Stmt { public: ExprPtr prompt; ExprPtr target; std::string limit; 
    ReadLine(ExprPtr p, ExprPtr t, std::string l) : prompt(std::move(p)), target(std::move(t)), limit(std::move(l)) {}
    std::string accept(class StmtVisitor& v) override;
};

class BreakStmt : public Stmt {
    std::string accept(class StmtVisitor& v) override;
};
class ContinueStmt : public Stmt {
    std::string accept(class StmtVisitor& v) override;
};
class ClearStmt : public Stmt {
    std::string accept(class StmtVisitor& v) override;
};

class RepeatCode : public Stmt {
public:
    ExprPtr value;
    std::vector<StmtPtr> body;

    RepeatCode(ExprPtr v, std::vector<StmtPtr> b) : value(std::move(v)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ForeverCode : public Stmt {
public:
    std::vector<StmtPtr> body;

    ForeverCode(std::vector<StmtPtr> b) : body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class CFDecl : public Stmt {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;

    CFDecl(std::string n, std::vector<Param> p, std::string r, std::vector<StmtPtr> b) : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ElifStmt : public Stmt {
public:
    ExprPtr condition;
    bool unl, lik = false;
    std::vector<StmtPtr> body;

    ElifStmt(ExprPtr c, bool u, bool l, std::vector<StmtPtr> b) : condition(std::move(c)), unl(u), lik(l), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ElseStmt : public Stmt {
public:
    bool unl, lik = false;
    std::vector<StmtPtr> body;

    ElseStmt(bool u, bool l, std::vector<StmtPtr> b) : unl(u), lik(l), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class IfStmt : public Stmt {
public:
    ExprPtr condition;
    bool unl, lik, eiunl, eilik, eunl, elik, iselif, iselse = false;
    std::vector<StmtPtr> body;
    ExprPtr elifCond;
    std::vector<StmtPtr> elifbody;
    std::vector<StmtPtr> elsebody;

    IfStmt(ExprPtr c, bool u, bool l, std::vector<StmtPtr> b, ExprPtr ec, std::vector<StmtPtr> eb, std::vector<StmtPtr> es) :
        condition(std::move(c)), unl(u), lik(l), body(std::move(b)), elifCond(std::move(ec)), elifbody(std::move(eb)), elsebody(std::move(es)) {}
    std::string accept(class StmtVisitor& v) override;
};



class WhileStmt : public Stmt {
public:
    ExprPtr condition;
    std::vector<StmtPtr> body;

    WhileStmt(ExprPtr c, std::vector<StmtPtr> b) : condition(std::move(c)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class DoStmt : public Stmt {
public:
    ExprPtr start;
    ExprPtr end;
    std::vector<StmtPtr> body;

    DoStmt(ExprPtr s, ExprPtr e, std::vector<StmtPtr> b) : start(std::move(s)), end(std::move(e)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ForRangeStmt : public Stmt {
public:
    bool shorte = false;
    std::string varName;
    ExprPtr condition;
    ExprPtr start;
    ExprPtr end;
    std::vector<StmtPtr> body;
    
    ForRangeStmt(bool s, std::string vn, ExprPtr c, ExprPtr st, ExprPtr e, std::vector<StmtPtr> b) : shorte(s), varName(std::move(vn)), condition(std::move(c)), start(std::move(st)), end(std::move(e)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class TryExcept : public Stmt {
public:
    std::vector<StmtPtr> tryBody;
    std::vector<StmtPtr> trybody_su;
    std::vector<StmtPtr> exceptbody_su;
    ExprPtr exceptCond;
    bool try_su = false, nec = false, except_su = false;
    bool hasExcept = false; // true only if an 'except' clause was actually written
    std::vector<StmtPtr> exceptBody;

    TryExcept(std::vector<StmtPtr> tb, std::vector<StmtPtr> tbsu, std::vector<StmtPtr> ebsu, ExprPtr ec, bool tsu, bool n, bool esu, bool he, std::vector<StmtPtr> eb) :
        tryBody(std::move(tb)), trybody_su(std::move(tbsu)), exceptbody_su(std::move(ebsu)), exceptCond(std::move(ec)), try_su(tsu), nec(n), except_su(esu), hasExcept(he), exceptBody(std::move(eb)) {}
    std::string accept(class StmtVisitor& v) override;
};

// ---------- Top level ----------

class FunctionDecl {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;
};

class CFuncDecl : public Stmt {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;

    CFuncDecl(std::string n, std::vector<Param> p, std::string r, std::vector<StmtPtr> b) : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};
class LambFuncDecl : public Stmt {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;

    LambFuncDecl(std::string n, std::vector<Param> p, std::string r, std::vector<StmtPtr> b) : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
    std::string accept(class StmtVisitor& v) override;
};

class ClassDecl {
public:
    std::string name;
    std::vector<StmtPtr> publicBody;
    std::vector<StmtPtr> privateBody;
    bool pub = false;
    bool pvr = false;
};

class ModuleDecl {
public:
    std::string name;
    std::vector<StmtPtr> body;
};

class StructCode {
public:
    std::string name;
    std::vector<StmtPtr> body;
};

class ClsPublic { public: std::vector<StmtPtr> body; };
class ClsPrivate { public: std::vector<StmtPtr> body; };

class LibImport { public: std::string libName; };

class Use { public: std::string first; std::string second; uint32_t mode; };
class AutoUse { public: std::string libName; uint32_t mode; };

class Program {
public:
    std::vector<LibImport> imports;
    std::vector<CFuncDecl> cfunctions;
    std::vector<FunctionDecl> functions;
    std::vector<ClassDecl> classes;
    std::vector<StructCode> struc;
    std::vector<Use> uses;
    std::vector<AutoUse> autouses;
    std::vector<ModuleDecl> modules;
    std::vector<TypeDecl> typedefs; // top-level 'ctype' aliases (declared outside any def/class/struct/module)
    bool use_built = true;
    std::unordered_set<std::string> usedObjects;
};

#endif