#include "ast.hpp"
#include "visitor.hpp"
#include <string>

// ---------- Expr ----------
std::string NumberLit::accept(ExprVisitor& v)          { return v.visit(*this); }
std::string StringLit::accept(ExprVisitor& v)          { return v.visit(*this); }
std::string CharLit::accept(ExprVisitor& v)            { return v.visit(*this); }
std::string LnQuote::accept(ExprVisitor& v)            { return v.visit(*this); }
std::string NameExpr::accept(ExprVisitor& v)           { return v.visit(*this); }
std::string ListLit::accept(ExprVisitor& v)            { return v.visit(*this); }
std::string FracLit::accept(ExprVisitor& v)            { return v.visit(*this); }
std::string IndexExpr::accept(ExprVisitor& v)          { return v.visit(*this); }
std::string CallExpr::accept(ExprVisitor& v)           { return v.visit(*this); }
std::string PostIncExpr::accept(ExprVisitor& v)        { return v.visit(*this); }
std::string PostMinExpr::accept(ExprVisitor& v)        { return v.visit(*this); }
std::string UnaryExpr::accept(ExprVisitor& v)          { return v.visit(*this); }
std::string BinaryExpr::accept(ExprVisitor& v)         { return v.visit(*this); }
std::string ConcatExpr::accept(ExprVisitor& v)         { return v.visit(*this); }
std::string MemberExpr::accept(ExprVisitor& v)         { return v.visit(*this); }
std::string MethodMemberExpr::accept(ExprVisitor& v)   { return v.visit(*this); }
std::string PointerExpr::accept(ExprVisitor& v)        { return v.visit(*this); }
std::string MethodCallExpr::accept(ExprVisitor& v)     { return v.visit(*this); }
std::string NamespaceCallExpr::accept(ExprVisitor& v)  { return v.visit(*this); }

// ---------- Stmt ----------
std::string VarDecl::accept(StmtVisitor& v)            { return v.visit(*this); }
std::string TypeDecl::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string AssignStmt::accept(StmtVisitor& v)         { return v.visit(*this); }
std::string ExprAssignStmt::accept(StmtVisitor& v)     { return v.visit(*this); }
std::string ReturnStmt::accept(StmtVisitor& v)         { return v.visit(*this); }
std::string ExprStmt::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string PrintCode::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string PrintMacCode::accept(StmtVisitor& v)       { return v.visit(*this); }
std::string ReadCode::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string ReadLine::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string BreakStmt::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string ContinueStmt::accept(StmtVisitor& v)       { return v.visit(*this); }
std::string ClearStmt::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string RepeatCode::accept(StmtVisitor& v)         { return v.visit(*this); }
std::string ForeverCode::accept(StmtVisitor& v)        { return v.visit(*this); }
std::string CFDecl::accept(StmtVisitor& v)             { return v.visit(*this); }
std::string ElifStmt::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string ElseStmt::accept(StmtVisitor& v)           { return v.visit(*this); }
std::string IfStmt::accept(StmtVisitor& v)             { return v.visit(*this); }
std::string WhileStmt::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string DoStmt::accept(StmtVisitor& v)             { return v.visit(*this); }
std::string ForRangeStmt::accept(StmtVisitor& v)       { return v.visit(*this); }
std::string TryExcept::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string CFuncDecl::accept(StmtVisitor& v)          { return v.visit(*this); }
std::string LambFuncDecl::accept(StmtVisitor& v)       { return v.visit(*this); }