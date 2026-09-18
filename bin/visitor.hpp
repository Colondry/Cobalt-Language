#ifndef VISITOR
#define VISITOR

#include <string>
#include <vector>

// Forward declarations — every class used in a visit() signature
class NumberLit;
class BinaryExpr;
class StringLit;
class CharLit;
class LnQuote;
class NameExpr;
class ListLit;
class FracLit;
class IndexExpr;
class CallExpr;
class PostIncExpr;
class PostMinExpr;
class UnaryExpr;
class ConcatExpr;
class MemberExpr;
class MethodCallExpr;
class MethodMemberExpr;
class NamespaceCallExpr;
class PointerExpr;

class AssignStmt;
class ExprAssignStmt;
class ReturnStmt;
class ExprStmt;
class VarDecl;
class TypeDecl;
class ReadCode;
class ReadLine;
class ForRangeStmt;
class IfStmt;
class WhileStmt;
class ElifStmt;
class ElseStmt;
class DoStmt;
class CFDecl;
class LambFuncDecl;
class CFuncDecl;
class ContinueStmt;
class BreakStmt;
class ClearStmt;
class PrintCode;
class PrintMacCode;
class RepeatCode;
class ForeverCode;
class TryExcept;

class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual std::string visit(NumberLit& n) { return ""; }
    virtual std::string visit(BinaryExpr& b) { return ""; }
    virtual std::string visit(StringLit& s) { return ""; }
    virtual std::string visit(CharLit& c) { return ""; }
    virtual std::string visit(LnQuote& l) { return ""; }
    virtual std::string visit(NameExpr& n) { return ""; }
    virtual std::string visit(ListLit& l) { return ""; }
    virtual std::string visit(FracLit& f) { return ""; }
    virtual std::string visit(IndexExpr& i) { return ""; }
    virtual std::string visit(CallExpr& c) { return ""; }
    virtual std::string visit(PostIncExpr& p) { return ""; }
    virtual std::string visit(PostMinExpr& p) { return ""; }
    virtual std::string visit(UnaryExpr& u) { return ""; }
    virtual std::string visit(ConcatExpr& c) { return ""; }
    virtual std::string visit(MemberExpr& m) { return ""; }
    virtual std::string visit(MethodCallExpr& m) { return ""; }
    virtual std::string visit(MethodMemberExpr& m) { return ""; }
    virtual std::string visit(NamespaceCallExpr& n) { return ""; }
    virtual std::string visit(PointerExpr& p) { return ""; }
};

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual std::string visit(AssignStmt& a) { return ""; }
    virtual std::string visit(ExprAssignStmt& e) { return ""; }
    virtual std::string visit(ReturnStmt& r) { return ""; }
    virtual std::string visit(ExprStmt& e) { return ""; }

    virtual std::string visit(VarDecl& v) { return ""; }
    virtual std::string visit(TypeDecl& t) { return ""; }
    virtual std::string visit(ReadCode& r) { return ""; }
    virtual std::string visit(ReadLine& r) { return ""; }
    virtual std::string visit(ForRangeStmt& f) { return ""; }
    virtual std::string visit(IfStmt& i) { return ""; }
    virtual std::string visit(WhileStmt& w) { return ""; }
    virtual std::string visit(ElifStmt& e) { return ""; }
    virtual std::string visit(ElseStmt& e) { return ""; }
    virtual std::string visit(DoStmt& d) { return ""; }
    virtual std::string visit(CFDecl& c) { return ""; }
    virtual std::string visit(LambFuncDecl& l) { return ""; }
    virtual std::string visit(CFuncDecl& c) { return ""; }
    virtual std::string visit(ContinueStmt& c) { return ""; }
    virtual std::string visit(BreakStmt& b) { return ""; }
    virtual std::string visit(ClearStmt& c) { return ""; }
    virtual std::string visit(PrintCode& p) { return ""; }
    virtual std::string visit(PrintMacCode& p) { return ""; }
    virtual std::string visit(RepeatCode& r) { return ""; }
    virtual std::string visit(ForeverCode& f) { return ""; }
    virtual std::string visit(TryExcept& t) { return ""; }
};


#endif