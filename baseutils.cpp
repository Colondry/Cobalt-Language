#include "baseutils.hpp"
#include <cstdlib>

// ---------- Type tokens ----------

bool Parser::isTypeToken(TokenType t) const {
    return t == TokenType::TypeInt || t == TokenType::TypeString || t == TokenType::TypeFloat ||
           t == TokenType::TypeDouble || t == TokenType::TypeByte || t == TokenType::TypeChar ||
           t == TokenType::TypeBool || t == TokenType::TypeVoid;
}

std::string Parser::typeName(TokenType t) {
    switch (t) {
        case TokenType::TypeInt: return "int";
        case TokenType::TypeString: return "string";
        case TokenType::TypeFloat: return "float";
        case TokenType::TypeDouble: return "double";
        case TokenType::TypeByte: return "byte";
        case TokenType::TypeChar: return "char";
        case TokenType::TypeBool: return "bool";
        case TokenType::TypeVoid: return "void";
        default: return "string";
    }
}

std::string Parser::expectType() {
    if (!isTypeToken(peek().type)) {
        reportError("expected a data type (int, string, float, double, byte, char, bool, void) but got '" +
                     peek().text + "'");
        advance();
        return "int";
    }
    return typeName(advance().type);
}

// ---------- Expressions ----------

ExprPtr Parser::parseBinaryLevel(const std::function<ExprPtr()>& parseNextLevel,
                                  const std::function<std::string(TokenType)>& operatorFor) {
    ExprPtr left = parseNextLevel();
    while (true) {
        std::string op = operatorFor(peek().type);
        if (op.empty()) break;
        advance();
        auto binary = std::make_shared<BinaryExpr>();
        binary->op = op;
        binary->lhs = left;
        binary->rhs = parseNextLevel();
        left = binary;
    }
    return left;
}

ExprPtr Parser::parseExpression() {
    ExprPtr base = parseLogicalOr();
    if (!check(TokenType::Shl) && !check(TokenType::Shr)) return base;

    auto chain = std::make_shared<ConcatExpr>();
    chain->pieces.push_back(base);
    while (check(TokenType::Shl) || check(TokenType::Shr)) {
        bool isShl = check(TokenType::Shl);
        advance();
        ExprPtr rhs = parseLogicalOr();
        if (isShl) chain->pieces.push_back(rhs);
        else chain->pieces.insert(chain->pieces.begin(), rhs);
    }
    return chain;
}

ExprPtr Parser::parseLogicalOr() {
    return parseBinaryLevel(
        [this] { return parseLogicalAnd(); },
        [](TokenType t) -> std::string { return t == TokenType::OrOr ? "||" : ""; });
}

ExprPtr Parser::parseLogicalAnd() {
    return parseBinaryLevel(
        [this] { return parseEquality(); },
        [](TokenType t) -> std::string { return t == TokenType::AndAnd ? "&&" : ""; });
}

ExprPtr Parser::parseEquality() {
    return parseBinaryLevel(
        [this] { return parseComparison(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Eq) return "==";
            if (t == TokenType::Neq) return "!=";
            return "";
        });
}

ExprPtr Parser::parseComparison() {
    return parseBinaryLevel(
        [this] { return parseAdditive(); },
        [](TokenType t) -> std::string {
            switch (t) {
                case TokenType::Lt: return "<";
                case TokenType::Gt: return ">";
                case TokenType::Le: return "<=";
                case TokenType::Ge: return ">=";
                default: return "";
            }
        });
}

ExprPtr Parser::parseAdditive() {
    return parseBinaryLevel(
        [this] { return parseMultiplicative(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Plus) return "+";
            if (t == TokenType::Minus) return "-";
            return "";
        });
}

ExprPtr Parser::parseMultiplicative() {
    return parseBinaryLevel(
        [this] { return parseUnary(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Star) return "*";
            if (t == TokenType::Slash) return "/";
            return "";
        });
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::Minus)) {
        advance();
        auto u = std::make_shared<UnaryExpr>();
        u->op = "-"; u->operand = parseUnary();
        return u;
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match(TokenType::LBracket)) {
            auto idx = std::make_shared<IndexExpr>();
            idx->base = expr;
            idx->index = parseExpression();
            expect(TokenType::RBracket, "]");
            expr = idx;
        } else if (check(TokenType::PlusPlus)) {
            if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
                advance();
                auto inc = std::make_shared<PostIncExpr>();
                inc->name = name->name;
                expr = inc;
            }
            break; // only one postfix ++ allowed
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::Number)) {
        auto lit = std::make_shared<NumberLit>();
        lit->value = advance().text;
        return lit;
    }
    if (check(TokenType::String)) {
        auto lit = std::make_shared<StringLit>();
        lit->value = advance().text;
        return lit;
    }
    if (check(TokenType::LBracket)) {
        advance();
        auto lit = std::make_shared<ListLit>();
        if (!check(TokenType::RBracket)) {
            lit->items.push_back(parseExpression());
            while (match(TokenType::Comma)) lit->items.push_back(parseExpression());
        }
        expect(TokenType::RBracket, "]");
        return lit;
    }
    if (check(TokenType::LParen)) {
        advance();
        ExprPtr inner = parseExpression();
        expect(TokenType::RParen, ")");
        return inner;
    }
    if (check(TokenType::Identifier)) {
        std::string name = advance().text;
        if (match(TokenType::LParen)) {
            auto call = std::make_shared<CallExpr>();
            call->callee = name;
            if (!check(TokenType::RParen)) {
                call->args.push_back(parseExpression());
                while (match(TokenType::Comma)) call->args.push_back(parseExpression());
            }
            expect(TokenType::RParen, ")");
            return call;
        }
        auto ref = std::make_shared<NameExpr>();
        ref->name = name;
        return ref;
    }

    reportError("expected an expression but got '" + peek().text + "'");
    advance();
    return std::make_shared<NumberLit>();
}

// ---------- Statements ----------

std::vector<StmtPtr> Parser::parseBlock() {
    expect(TokenType::LBrace, "{");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseStatement();
        if (s) stmts.push_back(s);
    }
    expect(TokenType::RBrace, "}");
    return stmts;
}

StmtPtr Parser::parseVarDecl() {
    if (check(TokenType::List)) {
        advance(); // 'List'
        auto decl = std::make_shared<VarDecl>();
        decl->type = "List";
        expect(TokenType::DoubleColon, "::");
        decl->elemType = expectType();
        Token nameTok = expect(TokenType::Identifier, "variable name");
        decl->name = nameTok.text;

        if (match(TokenType::Assign)) {
            expect(TokenType::LBracket, "[");
            auto lit = std::make_shared<ListLit>();
            if (!check(TokenType::RBracket)) {
                lit->items.push_back(parseExpression());
                while (match(TokenType::Comma)) lit->items.push_back(parseExpression());
            }
            expect(TokenType::RBracket, "]");
            decl->init = lit;
        }
        expect(TokenType::Semicolon, ";");
        return decl;
    }

    auto decl = std::make_shared<VarDecl>();
    decl->type = expectType();
    Token nameTok = expect(TokenType::Identifier, "variable name");
    decl->name = nameTok.text;

    if (match(TokenType::LBracket)) {
        Token sizeTok = expect(TokenType::Number, "array size");
        decl->arraySize = std::atoi(sizeTok.text.c_str());
        expect(TokenType::RBracket, "]");
    }
    if (match(TokenType::Assign)) {
        decl->init = parseExpression();
    }
    expect(TokenType::Semicolon, ";");
    return decl;
}

StmtPtr Parser::parseAssignOrExprStatement() {
    if (check(TokenType::Identifier) && current + 1 < tokens.size() &&
        tokens[current + 1].type == TokenType::Assign) {
        std::string name = advance().text;
        advance(); // '='
        auto stmt = std::make_shared<AssignStmt>();
        stmt->name = name;
        stmt->value = parseExpression();
        expect(TokenType::Semicolon, ";");
        return stmt;
    }

    ExprPtr expr = parseExpression();
    expect(TokenType::Semicolon, ";");
    auto stmt = std::make_shared<ExprStmt>();
    stmt->expr = expr;
    return stmt;
}

StmtPtr Parser::parseReturn() {
    advance(); // 'ret'
    auto stmt = std::make_shared<ReturnStmt>();
    if (!check(TokenType::Semicolon)) {
        stmt->value = parseExpression();
    }
    expect(TokenType::Semicolon, ";");
    return stmt;
}

StmtPtr Parser::parseIf() {
    advance(); // 'if'
    auto stmt = std::make_shared<IfStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock();
    return stmt;
}

StmtPtr Parser::parseWhile() {
    advance(); // 'while'
    auto stmt = std::make_shared<WhileStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock();
    return stmt;
}

StmtPtr Parser::parseForRange() {
    advance(); // 'for'
    Token nameTok = expect(TokenType::Identifier, "loop variable");
    expect(TokenType::In, "'in'");
    Token rangeTok = expect(TokenType::Identifier, "'range'");
    if (rangeTok.text != "range") {
        reportError("only 'range(...)' is supported in for-loops");
    }
    expect(TokenType::LParen, "(");
    auto stmt = std::make_shared<ForRangeStmt>();
    stmt->varName = nameTok.text;
    stmt->from = parseExpression();
    expect(TokenType::Comma, ",");
    stmt->to = parseExpression();
    expect(TokenType::RParen, ")");
    stmt->body = parseBlock();
    return stmt;
}

StmtPtr Parser::parsePrintCode() {
    bool newline = check(TokenType::PrintLine);
    advance(); // 'print' or 'println'
    expect(TokenType::LParen, "(");

    auto stmt = std::make_shared<PrintCode>();
    stmt->newline = newline;
    if (!check(TokenType::RParen)) {
        stmt->value = parseExpression();
    }
    expect(TokenType::RParen, ")");
    expect(TokenType::Semicolon, ";");
    return stmt;
}

StmtPtr Parser::parseInputCode() {
    advance(); // 'input'
    expect(TokenType::LParen, "(");

    auto stmt = std::make_shared<InputCode>();

    ExprPtr firstOperand = parseLogicalOr();

    if (check(TokenType::Shr)) {
        // input("prompt" >> var)
        advance(); // '>>'
        stmt->prompt = firstOperand;
        Token nameTok = expect(TokenType::Identifier, "variable name after '>>'");
        stmt->varName = nameTok.text;
    } else if (auto name = std::dynamic_pointer_cast<NameExpr>(firstOperand)) {
        // input(var
        stmt->varName = name->name;
    } else {
        reportError("expected a variable name inside input(...), like input(x) or input(\"prompt\" >> x)");
    }

    expect(TokenType::RParen, ")");
    expect(TokenType::Semicolon, ";");
    return stmt;
}

StmtPtr Parser::parseStatement() {
    // Add a new statement kind by adding one more check() here (or a
    // second isTypeToken()-style helper) and its own parseWhatever()
    // method above, following the same pattern as parseIf/parseWhile.
    if (isTypeToken(peek().type) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Ret)) return parseReturn();
    if (check(TokenType::If)) return parseIf();
    if (check(TokenType::While)) return parseWhile();
    if (check(TokenType::For)) return parseForRange();
    if (check(TokenType::Print) || check(TokenType::PrintLine)) return parsePrintCode();
    if (check(TokenType::Input)) return parseInputCode();
    if (check(TokenType::Identifier)) return parseAssignOrExprStatement();

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

// ---------- Top level ----------

void Parser::parseImport(Program& prog) {
    advance(); // '@'
    expect(TokenType::Import, "'import'");
    expect(TokenType::Lt, "<");
    Token lib = expect(TokenType::Identifier, "library name");
    expect(TokenType::Gt, ">");
    prog.imports.push_back({ lib.text });
}

FunctionDecl Parser::parseFunction() {
    advance(); // 'fn'
    Token nameTok = expect(TokenType::Identifier, "function name");
    expect(TokenType::LParen, "(");

    FunctionDecl fn;
    fn.name = nameTok.text;
    if (!check(TokenType::RParen)) {
        do {
            Param p;
            p.type = expectType();
            p.name = expect(TokenType::Identifier, "parameter name").text;
            fn.params.push_back(p);
        } while (match(TokenType::Comma));
    }
    expect(TokenType::RParen, ")");
    expect(TokenType::Colon, ":");
    fn.returnType = expectType();
    fn.body = parseBlock();
    return fn;
}