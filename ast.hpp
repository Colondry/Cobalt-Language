#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

// ---------- Expressions ----------

class Expr {
public:
    virtual ~Expr() = default;
};
using ExprPtr = std::shared_ptr<Expr>;

class NumberLit : public Expr { public: std::string value; };
class StringLit : public Expr { public: std::string value; }; // includes quotes
class CharLit : public Expr { public: std::string value; }; // includes single quotes
class NameExpr  : public Expr { public: std::string name; };
class ListLit    : public Expr { public: std::vector<ExprPtr> items; };
class IndexExpr  : public Expr { public: ExprPtr base; ExprPtr index; };
class CallExpr   : public Expr { public: std::string callee; std::vector<ExprPtr> args; };
class PostIncExpr: public Expr { public: std::string name; };
class UnaryExpr  : public Expr { public: std::string op; ExprPtr operand; };
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
    std::string type;      // int, string, float, double, byte, char, List
    std::string elemType;  // element type when type == "List"
    std::string name;
    int arraySize = -1;    // >=0 for "char c[20]"
    ExprPtr init;          // may be null (no initializer)
};

struct MemberExpr : Expr
{
    ExprPtr object;
    std::string member;
};

struct MethodCallExpr : Expr
{
    ExprPtr object;
    std::string method;
    std::vector<ExprPtr> args;
};

class AssignStmt : public Stmt { public: std::string name; ExprPtr value; };
class ReturnStmt  : public Stmt { public: ExprPtr value; }; // value may be null
class ExprStmt    : public Stmt { public: ExprPtr expr; };

class PrintCode : public Stmt { public: bool newline = false; ExprPtr value; int toRight = 1; };

class InputCode : public Stmt { public: ExprPtr prompt; std::string varName; };
class InputString : public Stmt { public: ExprPtr prompt; std::string varName; std::string limit; };

class BreakStmt : public Stmt {};
class ContinueStmt : public Stmt {};
class ClearStmt : public Stmt {};

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
    ExprPtr from;
    ExprPtr to;
    std::vector<StmtPtr> body;
}; 

// ---------- Top level ----------

class Param { public: std::string type; std::string name; };

class FunctionDecl {
public:
    std::string name;
    std::vector<Param> params;
    std::string returnType;
    std::vector<StmtPtr> body;
};

class LibImport { public: std::string libName; };

class Program {
public:
    std::vector<LibImport> imports;
    std::vector<FunctionDecl> functions;
    std::unordered_set<std::string> usedObjects;
};

#endif
