#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <cstdint>

// ---------- Expressions ----------

class Param { public: std::string type; std::string name; };

class Expr {
public:
    virtual ~Expr() = default;
};
using ExprPtr = std::shared_ptr<Expr>;

class NumberLit : public Expr { public: std::string value; };
class StringLit : public Expr { public: std::string value; }; // includes quotes
class CharLit : public Expr { public: std::string value; }; // includes single quotes
class LnQuote : public Expr { public: std::string value; }; // includes %"
class NameExpr : public Expr { public: std::string name; };
class ListLit : public Expr { public: std::vector<ExprPtr> items; double index; };
class FracLit : public Expr { public: std::vector<ExprPtr> items; double index; };
class IndexExpr : public Expr { public: ExprPtr base; ExprPtr index; };
class CallExpr : public Expr { public: std::string callee; std::vector<ExprPtr> args; };
class PostIncExpr : public Expr { public: std::string name; };
class PostMinExpr : public Expr { public: std::string name; };
class UnaryExpr : public Expr { public: std::string op; ExprPtr operand; };
class BinaryExpr : public Expr { public: std::string op; ExprPtr lhs; ExprPtr rhs; };

class ConcatExpr : public Expr {
public:
    std::vector<ExprPtr> pieces;
};

// ---------- Statements ----------

class Stmt { public: virtual ~Stmt() = default; };
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
};
class TypeDecl : public Stmt {
public:
    std::string type;         // int, string, float, double, byte, char, List
    std::string elemType;     // element type when type == "List"
    std::string secElemType;  // element type when type == "frac"
    std::string name;         // name of the variable
    int arraySize = -1;       // >=0 for "char c[20]"
};

struct MemberExpr : Expr
{
    ExprPtr object;
    std::string member;
};
struct MethodMemberExpr : Expr
{
    ExprPtr object;
    std::string member;
};
struct PointerExpr : Expr {
    ExprPtr object;
    std::string member;
};

struct MethodCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;
};
struct NamespaceCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;
};

class AssignStmt : public Stmt { public: std::string name; ExprPtr value; };
class ExprAssignStmt : public Stmt { public: ExprPtr target; ExprPtr value; };
class ReturnStmt : public Stmt { public: ExprPtr value; }; // value may be null
class ExprStmt : public Stmt { public: ExprPtr expr; };

class PrintCode : public Stmt { public: bool newline = false; ExprPtr value; int toRight = 1; };
class PrintMacCode : public Stmt { public: bool newline = false; ExprPtr value; };

class ReadCode : public Stmt { public: ExprPtr prompt; ExprPtr target; };
class ReadLine : public Stmt { public: ExprPtr prompt; ExprPtr target; std::string limit; };

class BreakStmt : public Stmt {};
class ContinueStmt : public Stmt {};
class ClearStmt : public Stmt {};

class RepeatCode : public Stmt {
public:
    ExprPtr value;
    std::vector<StmtPtr> body;
};

class ForeverCode : public Stmt {
public:
    std::vector<StmtPtr> body;
};

class CFDecl : public Stmt {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;
};

class IfStmt : public Stmt {
public:
    ExprPtr condition;
    std::vector<StmtPtr> body;
};

class ElifStmt : public Stmt {
public:
    ExprPtr condition;
    std::vector<StmtPtr> body;
};

class ElseStmt : public Stmt {
public:
    std::vector<StmtPtr> body;
};

class WhileStmt : public Stmt {
public:
    ExprPtr condition;
    std::vector<StmtPtr> body;
};

class ForRangeStmt : public Stmt {
public:
    std::string varName;
    ExprPtr condition;
    std::vector<StmtPtr> body;
};

class TryExcept : public Stmt {
public:
    std::vector<StmtPtr> tryBody;
    std::vector<StmtPtr> trybody_su;
    std::vector<StmtPtr> exceptbody_su;
    ExprPtr exceptCond;
    bool try_su, nec, except_su;
    std::vector<StmtPtr> exceptBody;
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
};
class LambFuncDecl : public Stmt {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;
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