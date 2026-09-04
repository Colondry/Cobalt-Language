#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

enum class VariableState { Active, Moved };
enum class VariableConst { Nothing, Constant, CPointer, Both };

class Parser
{
public:
    Program program;
    Parser() = default;
    explicit Parser(std::vector<Token> tokens, std::string source = "", std::string inputFileDir = "");

    // Entry point: parses the whole token stream into a Program.
    Program parse();

    std::vector<std::string> errors;


private:

    std::vector<Token> tokens;
    std::string source;
    std::string inputFileDir; // directory of the .cb file being compiled

    size_t current = 0;
    std::unordered_set<std::string> ctypeNames; // names introduced via 'ctype'

    std::vector<std::unordered_map<std::string, VariableState>> scopes;
    std::vector<std::unordered_map<std::string, VariableConst>> const_scopes;

    inline void pushScope() { scopes.push_back({}); }
    inline void popScope() { if (!scopes.empty()) scopes.pop_back(); }

    inline void pushConst() { const_scopes.push_back({}); }
    inline void popConst() { if(!const_scopes.empty()) const_scopes.pop_back(); }

    void declareVar(const std::string& name, int conf = 0) {
        if (!scopes.empty()) scopes.back()[name] = VariableState::Active;
        if (!const_scopes.empty()) {
            VariableConst vc = VariableConst::Nothing;
            switch (conf) {
                case 0: vc = VariableConst::Nothing; break;
                case 1: vc = VariableConst::Constant; break;
                case 2: vc = VariableConst::CPointer; break;
                case 3: vc = VariableConst::Both; break;

                default: reportError("unknown panic! {404}"); break;
            }
            const_scopes.back()[name] = vc;
        }
    }

    VariableConst getVarConstState(const std::string& name) {
        for (auto it = const_scopes.rbegin(); it != const_scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return VariableConst::Nothing;
    }

    VariableState getVarState(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return VariableState::Active; // Fallback for globals/members
    }

    void setVarState(const std::string& name, VariableState state) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                found->second = state;
                return;
            }
        }
    }

    // ---------- Core cursor / matching infra (defined in parser.cpp) ----------
    const Token& peek() const;
    const Token& peekNext() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& what);
    void reportError(const std::string& message);
    void recoverStatement();

    // ---------- Types (defined in baseutils.cpp) ----------
    bool isTypeToken(TokenType t, std::string type);
    bool looksLikeVarDecl(); // disambiguates "StructName varname" (decl) from "StructName.field = ..." (stmt on the singleton)
    std::string typeName(std::string type, TokenType t);
    std::string expectType();

    // ---------- Expressions, lowest to highest precedence (baseutils.cpp) ---------
    ExprPtr parseBinaryLevel(const std::function<ExprPtr()>& parseNextLevel,
        const std::function<std::string(TokenType)>& operatorFor);
    ExprPtr parseExpression();     // handles '<<' / '>>' string concatenation
    ExprPtr parseMacExpression();     // handles ',' string concatenation
    ExprPtr parseLogicalOr();      // ||
    ExprPtr parseLogicalAnd();     // &&
    ExprPtr parseEquality();       // == !=
    ExprPtr parseComparison();     // < > <= >=
    ExprPtr parseAdditive();       // + -
    ExprPtr parseMultiplicative(); // * /
    ExprPtr parseUnary();          // unary -
    ExprPtr parsePostfix();        // x[i], x++
    ExprPtr parsePrimary();        // literals, ( ), names, calls
    StmtPtr parseAssignAMMS();     // +=, -=, *=, /=

    // ---------- Statements (baseutils.cpp) ----------
    std::vector<StmtPtr> parseBlock(std::string retype);
    std::vector<StmtPtr> parseInlineBlock(std::string retype);
    std::vector<StmtPtr> parseCFBlock(std::string retype);
    std::vector<StmtPtr> parseSFBlock();
    void parseSBlock(StructCode& str);
    void parseCBlock(ClassDecl& cls);
    void parseModuleBlock(ModuleDecl& mod);
    StmtPtr parseVarDecl(int c = 0); // 0 = !const, 1 = with const, 2 = with const_ptr, 3 = both
    StmtPtr parseAssignOrExprStatement();
    StmtPtr parseReturn(std::string retype);
    StmtPtr parseIf();
    StmtPtr parseElse();
    StmtPtr parseElif();
    StmtPtr parseWhile();
    StmtPtr parseRepeat();
    StmtPtr parseForever();
    StmtPtr parseForRange();
    StmtPtr parseStatement(std::string retype);
    StmtPtr parseSClass();
    StmtPtr parseSStr();
    StmtPtr parsePrintCode();
    StmtPtr parseReadCode();
    StmtPtr parseReadLineCode();
    StmtPtr parseContinue();
    StmtPtr parseBreak();
    StmtPtr parseClear();
    StmtPtr parsePrintMac();
    StmtPtr parseTryExcept();
    StmtPtr parseCType();
    TypeDecl parseCTypeBody(); // shared by parseCType() (statement) and parseNCType() (top-level)


    // ---------- Top level (baseutils.cpp) ----------
    void parseImport(Program& prog);
    FunctionDecl parseFunction();
    StmtPtr parseLambdaFn(std::string retype);
    StmtPtr parseCFunction();
    ClassDecl parseClasses();
    ClsPublic parsePublic();
    ClsPrivate parsePrivate();
    StructCode parseStruct();
    Use parseUse();
    AutoUse parseAutoUse();
    ModuleDecl parseModule();
    bool parsenUse();
    TypeDecl parseNCType(); // 'ctype' declared outside any def/class/struct/module
};

#endif