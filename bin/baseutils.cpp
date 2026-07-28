#include "baseutils.hpp"
#include "ast.hpp"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_set>

// ---------- Type tokens ----------

bool Parser::isTypeToken(TokenType t) const {
    return t == TokenType::TypeInt || t == TokenType::TypeString || t == TokenType::TypeFloat ||
        t == TokenType::TypeDouble || t == TokenType::TypeByte || t == TokenType::TypeChar ||
        t == TokenType::TypeBool || t == TokenType::TypeVoid || t == TokenType::TypeAuto   
        ||  t == TokenType::TypeLong;
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
    case TokenType::TypeAuto: return "auto";
    case TokenType::TypeLong: return "long double";
    default: return "auto";
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

// Helper for parsing binary expressions with a given precedence level.
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

// --------- Expression parsing, lowest to highest precedence ----------
ExprPtr Parser::parseExpression() {
    ExprPtr base = parseLogicalOr();
    if (!check(TokenType::Shl) && !check(TokenType::Shr)) return base;

    auto chain = std::make_shared<ConcatExpr>();
    chain->pieces.push_back(base);
    while (check(TokenType::Shl) || check(TokenType::Shr)) {
        bool isShl = check(TokenType::Shl);
        advance();
        ExprPtr rhs = parseLogicalOr();
        if (isShl) chain->pieces.insert(chain->pieces.begin(), rhs);  // '<<' -> starts from the right
        else chain->pieces.push_back(rhs);                            // '>>' -> starts from the left
    }
    return chain;
}

// Parses a logical OR expression (||)
ExprPtr Parser::parseLogicalOr() {
    return parseBinaryLevel(
        [this] { return parseLogicalAnd(); },
        [](TokenType t) -> std::string { return t == TokenType::OrOr ? "||" : ""; });
}

// Parses a logical AND expression (&&)
ExprPtr Parser::parseLogicalAnd() {
    return parseBinaryLevel(
        [this] { return parseEquality(); },
        [](TokenType t) -> std::string { return t == TokenType::AndAnd ? "&&" : ""; });
}

// Parses an equality expression (==, !=)
ExprPtr Parser::parseEquality() {
    return parseBinaryLevel(
        [this] { return parseComparison(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Eq) return "==";
            if (t == TokenType::Neq) return "!=";
            return "";
        });
}

// Parses a comparison expression (<, >, <=, >=)
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

// Parses an additive expression (+, -)
ExprPtr Parser::parseAdditive() {
    return parseBinaryLevel(
        [this] { return parseMultiplicative(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Plus) return "+";
            if (t == TokenType::Minus) return "-";
            return "";
        });
}

// Parses a multiplicative expression (*, /)
ExprPtr Parser::parseMultiplicative() {
    return parseBinaryLevel(
        [this] { return parseUnary(); },
        [](TokenType t) -> std::string {
            if (t == TokenType::Star) return "*";
            if (t == TokenType::Slash) return "/";
            return "";
        });
}

// Parses a unary expression (-, !)
ExprPtr Parser::parseUnary() {
    if (check(TokenType::Minus)) {
        advance();
        auto u = std::make_shared<UnaryExpr>();
        u->op = "-"; u->operand = parseUnary();
        return u;
    }
    return parsePostfix();
}

// Parses a postfix expression (x[i], x++)
StmtPtr Parser::parseAssignAMMS() {
    if (check(TokenType::Identifier) && current + 1 < tokens.size()) {
        TokenType nextType = tokens[current + 1].type;
        if (nextType == TokenType::AssignAdd || nextType == TokenType::AssignMinus ||
            nextType == TokenType::AssignMulti || nextType == TokenType::AssignSlash) {
            std::string name = advance().text;
            std::string op = advance().text; // +=, -=, *=, /=
            auto stmt = std::make_shared<AssignStmt>();
            stmt->name = name;
            auto binary = std::make_shared<BinaryExpr>();
            binary->op = op.substr(0, 1); // convert "+=" to "+", "-=" to "-", etc.
            auto nameExpr = std::make_shared<NameExpr>();
            nameExpr->name = name;
            binary->lhs = nameExpr;
            binary->rhs = parseExpression();
            stmt->value = binary;
            return stmt;
        }
    }
    return nullptr;
}

//---------- Primary expressions ----------
ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match(TokenType::LBracket)) { // ()
            auto idx = std::make_shared<IndexExpr>();
            idx->base = expr;
            idx->index = parseExpression();
            expect(TokenType::RBracket, "]");
            expr = idx;
        }
        else if (check(TokenType::PlusPlus)) { // ++
            if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
                advance();
                auto inc = std::make_shared<PostIncExpr>();
                inc->name = name->name;
                expr = inc;
            }
            else {
                reportError("'++' can only be applied to a variable name");
                advance(); // consume the '++' so we don't loop forever
                break;
            }
        }
        else if (check(TokenType::MinusMinus)) { // ++
            if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
                advance();
                auto inc = std::make_shared<PostIncExpr>();
                inc->name = name->name;
                expr = inc;
            }
            else {
                reportError("'--' can only be applied to a variable name");
                advance(); // consume the '++' so we don't loop forever
                break;
            }
        }
        else if (match(TokenType::Dot)) {
            Token member =
                expect(TokenType::Identifier, "member name after '.'");

            if (match(TokenType::LParen))
            {
                auto call =
                    std::make_shared<MethodCallExpr>();

                call->object = expr; // the object on which the method is called
                call->method = member.text; // the method name

                if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
                    program.usedObjects.insert(name->name);
                }

                if (!check(TokenType::RParen)) {
                    do {
                        call->args.push_back(parseExpression());
                    } while (match(TokenType::Comma));
                }
                expect(TokenType::RParen, ")");
                expr = call;
            }
            else {
                auto m =
                    std::make_shared<MemberExpr>();

                m->object = expr;
                m->member = member.text;

                expr = m;
            }
        }
        else {
            break;
        }
    }
    return expr;
}

// Parses a primary expression
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
    if (check(TokenType::Char)) {
        auto lit = std::make_shared<CharLit>();
        lit->value = "'" + advance().text + "'";
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
        expect(TokenType::Lt, "<");
        decl->elemType = expectType();
        expect(TokenType::Gt, ">");
        Token nameTok = expect(TokenType::Identifier, "variable name");
        decl->name = nameTok.text;

        if (match(TokenType::Assign)) {
            expect(TokenType::LBracket, "[");
            auto lit = std::make_shared<ListLit>();
            if (!check(TokenType::RBracket)) {
                lit->items.push_back(parseExpression()); lit->index++;
                while (match(TokenType::Comma))
                {
                    lit->items.push_back(parseExpression());
                    lit->index++;
                }
            }
            expect(TokenType::RBracket, "]");
            if (check(TokenType::Semicolon)) {
                advance();
            }
            decl->init = lit;
        }
        return decl;
    }
    else if (check(TokenType::TypeVoid)) {
        reportError("variable type cannot be void.");
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
    else if (!check(TokenType::Assign)) {
        reportError("variable '" + decl->name + "' must be initialized -- declarations cannot be left without a value");
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
    else {
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
    return decl;
}

StmtPtr Parser::parseAssignOrExprStatement() {
    if (auto amms = parseAssignAMMS()) {
        if (check(TokenType::Semicolon)) {
            advance();
        }
        return amms;
    }

    if (check(TokenType::Identifier) && current + 1 < tokens.size() &&
        tokens[current + 1].type == TokenType::Assign) {
        std::string name = advance().text;
        advance(); // '='
        auto stmt = std::make_shared<AssignStmt>();
        stmt->name = name;
        stmt->value = parseExpression();
        if (check(TokenType::Semicolon)) {
            advance();
        }
        return stmt;
    }

    ExprPtr expr = parseExpression();
    advance();
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
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseIf() {
    advance(); // 'if'
    auto stmt = std::make_shared<IfStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock();
    return stmt;
}

StmtPtr Parser::parseElif() {
    advance(); // 'elif'
    auto stmt = std::make_shared<ElifStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock();
    return stmt;
}

StmtPtr Parser::parseElse() {
    advance(); // 'else'
    auto stmt = std::make_shared<ElseStmt>();
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
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseReadCode() {
    advance(); // 'read'
    expect(TokenType::LParen, "(");

    auto stmt = std::make_shared<ReadCode>();

    ExprPtr firstOperand = parseLogicalOr();

    if (check(TokenType::Shr)) {
        // input("prompt" >> var)
        advance(); // '>>'
        stmt->prompt = firstOperand;
        Token nameTok = expect(TokenType::Identifier, "variable name after '>>'");
        stmt->varName = nameTok.text;
    }
    else if (auto name = std::dynamic_pointer_cast<NameExpr>(firstOperand)) {
        // input(var
        stmt->varName = name->name;
    }
    else {
        reportError("expected a variable name inside input(...), like input(x) or input(\"prompt\" >> x)");
    }

    expect(TokenType::RParen, ")");
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseReadLineCode() {
    advance(); // 'readln'
    expect(TokenType::LParen, "(");

    auto stmt = std::make_shared<ReadLine>();

    ExprPtr firstOperand = parseLogicalOr();

    if (check(TokenType::Shr)) {
        // inputstr("prompt" >> var)
        advance(); // '>>'
        stmt->prompt = firstOperand; // any expression is fine as a prompt, not just string literals
        Token nameTok = expect(TokenType::Identifier, "variable name after '>>'");
        stmt->varName = nameTok.text;
    }
    else if (auto name = std::dynamic_pointer_cast<NameExpr>(firstOperand)) {
        // inputstr(var
        stmt->varName = name->name;
    }
    else {
        reportError("expected a variable name inside inputstr(...), like inputstr(x) or inputstr(\"prompt\" >> x)");
    }

    if (match(TokenType::Comma)) {
        // inputstr(..., 'limit')
        if (!check(TokenType::Char)) {
            reportError("expected a char literal for the limit in inputstr(..., limit)");
        }
        ExprPtr limitExpr = parseExpression();
        if (auto limitLit = std::dynamic_pointer_cast<CharLit>(limitExpr)) {
            stmt->limit = limitLit->value;
        }
        else {
            reportError("expected a char literal for the limit in inputstr(..., limit)");
        }
    }
    else {
        stmt->limit = ""; // no limit specified
    }

    expect(TokenType::RParen, ")");
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseContinue() {
    advance(); // 'continue'
    auto stmt = std::make_shared<ContinueStmt>();
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseBreak() {
    advance(); // 'break'
    auto stmt = std::make_shared<BreakStmt>();
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

StmtPtr Parser::parseClear() {
    advance(); // clear
    auto stmt = std::make_shared<ClearStmt>();
    expect(TokenType::LParen, "("); expect(TokenType::RParen, ")");
    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

std::vector<StmtPtr> Parser::parseCFBlock() {
    expect(TokenType::LBrace, "{");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::SClose) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseStatement();
        if (s) stmts.push_back(s);
    }
    if (check(TokenType::SClose)) {
        advance(); // "};"
    }
    else {
        expect(TokenType::RBrace, "}");
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
    return stmts;
}
std::vector<StmtPtr> Parser::parseSFBlock() {
    expect(TokenType::LBrace, "{");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::SClose) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseSStr();
        if (s) stmts.push_back(s);
    }
    if (check(TokenType::SClose)) {
        advance(); // "};"
    }
    else {
        expect(TokenType::RBrace, "}");
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
    return stmts;
}

StmtPtr Parser::parseCFunction() {
    advance(); // 'fn'
    Token nameTok = expect(TokenType::Identifier, "function name");
    expect(TokenType::LParen, "(");

    CFuncDecl fn;
    auto fnd = std::make_shared<CFDecl>();
    fn.name = nameTok.text; fnd->name = nameTok.text;
    if (!check(TokenType::RParen)) {
        do {
            Param p;
            p.type = expectType();
            p.name = expect(TokenType::Identifier, "parameter name").text;
            fn.params.push_back(p); fnd->params.push_back(p);
        } while (match(TokenType::Comma));
    }
    expect(TokenType::RParen, ")");
    expect(TokenType::Colon, ":");
    fn.returnType = expectType(); fnd->returnType = fn.returnType;
    fnd->body = parseCFBlock();
    fn.body = fnd->body;

    return fnd;
}

StmtPtr Parser::parseStatement() {
    if (isTypeToken(peek().type) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Ret)) return parseReturn();
    if (check(TokenType::If)) return parseIf();
    if (check(TokenType::Elif)) return parseElif();
    if (check(TokenType::Else)) return parseElse();
    if (check(TokenType::While)) return parseWhile();
    if (check(TokenType::For)) return parseForRange();
    if (check(TokenType::Print) || check(TokenType::PrintLine)) return parsePrintCode();
    if (check(TokenType::Read)) return parseReadCode();
    if (check(TokenType::Continue)) return parseContinue();
    if (check(TokenType::Break)) return parseBreak();
    if (check(TokenType::Clear)) return parseClear();
    if (check(TokenType::ReadLine)) return parseReadLineCode();
    if (check(TokenType::Identifier)) return parseAssignOrExprStatement();

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

StmtPtr Parser::parseSStr() {
    if (isTypeToken(peek().type) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Identifier)) return parseAssignOrExprStatement();

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

StmtPtr Parser::parseSClass() {
    if (isTypeToken(peek().type) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Fn)) return parseCFunction();
    if (check(TokenType::Ret)) return parseReturn();
    if (check(TokenType::If)) return parseIf();
    if (check(TokenType::Elif)) return parseElif();
    if (check(TokenType::Else)) return parseElse();
    if (check(TokenType::While)) return parseWhile();
    if (check(TokenType::For)) return parseForRange();
    if (check(TokenType::Print) || check(TokenType::PrintLine)) return parsePrintCode();
    if (check(TokenType::Read)) return parseReadCode();
    if (check(TokenType::Continue)) return parseContinue();
    if (check(TokenType::Break)) return parseBreak();
    if (check(TokenType::Clear)) return parseClear();
    if (check(TokenType::ReadLine)) return parseReadLineCode();
    if (check(TokenType::Identifier)) return parseAssignOrExprStatement();

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

void Parser::parseCBlock(ClassDecl& cls) {
    expect(TokenType::LBrace, "{");
    while (!check(TokenType::RBrace) && !check(TokenType::SClose) && !check(TokenType::EndOfFile)) {
        if (check(TokenType::Public)) // public {}
        {
            cls.pub = true;
            advance(); // public
            expect(TokenType::LBrace, "{");
            while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
                StmtPtr s = parseSClass();
                if (s) cls.publicBody.push_back(s);
            }
            expect(TokenType::RBrace, "}");
        }
        else if (check(TokenType::Private)) // private {}
        {
            cls.pvr = true;
            advance(); // private
            expect(TokenType::LBrace, "{");
            while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
                StmtPtr s = parseSClass();
                if (s) cls.privateBody.push_back(s);
            }
            expect(TokenType::RBrace, "}");
        }
        else {
            reportError("expected 'public' or 'private' block inside class '" + cls.name + "' but got '" + peek().text + "'");
            recoverStatement();
            break;
        }
    }
    if (check(TokenType::SClose)) {
        advance(); // "};"
    }
    else {
        expect(TokenType::RBrace, "}");
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
}

void Parser::parseSBlock(StructCode& str) {
    expect(TokenType::LBrace, "{");
    while (!check(TokenType::RBrace) && !check(TokenType::SClose) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseSClass();
        if (s) str.body.push_back(s);
    }
    if (check(TokenType::SClose)) {
        advance(); // "};"
    }
    else {
        expect(TokenType::RBrace, "}");
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
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
    if (check(TokenType::Colon)) {
        advance();
        fn.returnType = expectType();
    }
    else {
        if (fn.name == "main") {
            fn.returnType = "int";
        }
        else {
            fn.returnType = "auto";
        }
    }
    fn.body = parseBlock();
    return fn;
}

ClassDecl Parser::parseClasses() {
    advance(); // class
    Token nameCls = expect(TokenType::Identifier, "class name");

    ClassDecl cls;
    cls.name = nameCls.text;
    parseCBlock(cls);

    return cls;
}

StructCode Parser::parseStruct() {
    advance(); // struct
    Token nameStr = expect(TokenType::Identifier, "struct name");

    StructCode str;
    str.name = nameStr.text;
    parseSBlock(str);

    return str;
}