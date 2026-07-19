#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>

// ---------- Expressions ----------

class Expr {
public:
    virtual ~Expr() = default;
};
using ExprPtr = std::shared_ptr<Expr>;

class NumberLit : public Expr { public: std::string value; };
class StringLit : public Expr { public: std::string value; }; // includes quotes
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

class AssignStmt : public Stmt { public: std::string name; ExprPtr value; };
class ReturnStmt  : public Stmt { public: ExprPtr value; }; // value may be null
class ExprStmt    : public Stmt { public: ExprPtr expr; };

class PrintCode : public Stmt { public: bool newline = false; ExprPtr value; };

class InputCode : public Stmt { public: ExprPtr prompt; std::string varName; };

class IfStmt : public Stmt {
public:
    ExprPtr condition;
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
};

#endif