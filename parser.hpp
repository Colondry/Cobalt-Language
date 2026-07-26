#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

class Parser
{
public:
    Program program;
    Parser() = default;
    explicit Parser(std::vector<Token> tokens, std::string source = "");

    // Entry point: parses the whole token stream into a Program.
    Program parse();

    std::vector<std::string> errors;


private:

    std::vector<Token> tokens;
    std::string source;

    size_t current = 0;

    // ---------- Core cursor / matching infra (defined in parser.cpp) ----------
    const Token& peek() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& what);
    void reportError(const std::string& message);
    void recoverStatement();

    // ---------- Types (defined in baseutils.cpp) ----------
    bool isTypeToken(TokenType t) const;
    std::string typeName(TokenType t);
    std::string expectType();

    // ---------- Expressions, lowest to highest precedence (baseutils.cpp) ---------
    ExprPtr parseBinaryLevel(const std::function<ExprPtr()>& parseNextLevel,
        const std::function<std::string(TokenType)>& operatorFor);
    ExprPtr parseExpression();     // handles '<<' / '>>' string concatenation
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
    std::vector<StmtPtr> parseBlock();
    std::vector<StmtPtr> parseCFBlock();
    std::vector<StmtPtr> parseSFBlock();
    void parseSBlock(StructCode& str);
    void parseCBlock(ClassDecl& cls);
    StmtPtr parseVarDecl();
    StmtPtr parseAssignOrExprStatement();
    StmtPtr parseReturn();
    StmtPtr parseIf();
    StmtPtr parseElse();
    StmtPtr parseElif();
    StmtPtr parseWhile();
    StmtPtr parseForRange();
    StmtPtr parseStatement();
    StmtPtr parseSClass();
    StmtPtr parseSStr();
    StmtPtr parsePrintCode();
    StmtPtr parseReadCode();
    StmtPtr parseReadLineCode();
    StmtPtr parseContinue();
    StmtPtr parseBreak();
    StmtPtr parseClear();

    // ---------- Top level (baseutils.cpp) ----------
    void parseImport(Program& prog);
    FunctionDecl parseFunction();
    StmtPtr parseCFunction();
    ClassDecl parseClasses();
    ClsPublic parsePublic();
    ClsPrivate parsePrivate();
    StructCode parseStruct();
};

#endif
