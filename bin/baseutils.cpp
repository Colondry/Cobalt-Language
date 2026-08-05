#include "baseutils.hpp"
#include "ast.hpp"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <unordered_set>

// ---------- Type tokens ----------

bool Parser::isTypeToken(TokenType t, std::string type) {
    for (const StructCode& sc : program.struc) {
        if (sc.name == type) {
            return true;
        }
    }
    return t == TokenType::TypeInt || t == TokenType::TypeString || t == TokenType::TypeFloat ||
        t == TokenType::TypeDouble || t == TokenType::TypeByte || t == TokenType::TypeChar ||
        t == TokenType::TypeBool || t == TokenType::TypeVoid || t == TokenType::TypeAuto
        || t == TokenType::TypeInt8 || t == TokenType::TypeInt16 || t == TokenType::TypeInt32
        || t == TokenType::TypeInt64 || t == TokenType::TypeFrac;
}

std::string Parser::typeName(std::string type, TokenType t) {
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
    case TokenType::TypeInt8: return "std::uint8_t";
    case TokenType::TypeInt16: return "std::uint16_t";
    case TokenType::TypeInt32: return "std::uint32_t";
    case TokenType::TypeInt64: return "std::uint64_t";
    default:
        for (const StructCode& sc : program.struc) {
            if (sc.name == type) {
                return "__" + type + "__";
            }
        }
        return peek().text;
    }
}

std::string Parser::expectType() {
    std::string ty = peek().text;
    if (!isTypeToken(peek().type, ty)) {
        reportError("expected a data type (int, string, float, double, byte, char, bool, void) but got '" +
            peek().text + "'");
        advance();
        return "auto";
    }
    return typeName(ty, advance().type);
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
ExprPtr Parser::parseMacExpression() {
    ExprPtr base = parseLogicalOr();
    if (!check(TokenType::Comma)) return base;

    auto chain = std::make_shared<ConcatExpr>();
    chain->pieces.push_back(base);
    while (check(TokenType::Comma)) {
        advance();
        ExprPtr rhs = parseLogicalOr();
        chain->pieces.push_back(rhs);
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
        else if (check(TokenType::MinusMinus)) { // --
            if (auto name = std::dynamic_pointer_cast<NameExpr>(expr)) {
                advance();
                auto dec = std::make_shared<PostMinExpr>();
                dec->name = name->name;
                expr = dec;
            }
            else {
                reportError("'--' can only be applied to a variable name");
                advance(); // consume the '--' so we don't loop forever
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

std::vector<StmtPtr> Parser::parseBlock(std::string retype) {
    expect(TokenType::LBrace, "{");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseStatement(retype);
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
        else {
            reportError("variable '" + decl->name + "' must be initialized -- declarations cannot be left without a value");
            if (check(TokenType::Semicolon)) {
                advance();
            }
        }
        return decl;
    }
    else if (check(TokenType::TypeFrac)) {
        advance(); // 'frac'
        auto decl = std::make_shared<VarDecl>();
        decl->type = "Fraction";
        expect(TokenType::Lt, "<");
        decl->elemType = expectType();
        if (check(TokenType::Comma)) {
            advance();
            decl->secElemType = expectType();
        }
        else {
            decl->secElemType = decl->elemType;
        }
        expect(TokenType::Gt, ">");
        Token nameTok = expect(TokenType::Identifier, "variable name");
        decl->name = nameTok.text;

        if (match(TokenType::Assign)) {
            expect(TokenType::LBracket, "[");
            auto lit = std::make_shared<FracLit>();
            if (!check(TokenType::RBracket)) {
                lit->items.push_back(parseExpression()); lit->index++;
                if (lit->index > 2) {
                    reportError("fraction index cannot be more than 2.");
                }
                while (match(TokenType::Comma))
                {
                    lit->items.push_back(parseExpression());
                    lit->index++;
                    if (lit->index > 2) {
                        reportError("fraction index cannot be more than 2.");
                    }
                }
                if (lit->index > 2) {
                    reportError("fraction index cannot be more than 2.");
                }
            }
            expect(TokenType::RBracket, "]");
            if (check(TokenType::Semicolon)) {
                advance();
            }
            decl->init = lit;
        }
        else {
            reportError("variable '" + decl->name + "' must be initialized -- declarations cannot be left without a value");
            if (check(TokenType::Semicolon)) {
                advance();
            }
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
        if (check(TokenType::Semicolon)) {
            advance();
        }
    }
    else {
        reportError("variable '" + decl->name + "' must be initialized -- declarations cannot be left without a value");
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

    // `object.field = value` (MemberExpr) or `list[i] = value` (IndexExpr)
    // -- the bare-identifier fast path above only catches `name = value`,
    // so a general lvalue expression followed by '=' lands here instead.
    if ((std::dynamic_pointer_cast<MemberExpr>(expr) || std::dynamic_pointer_cast<IndexExpr>(expr))
        && check(TokenType::Assign)) {
        advance(); // '='
        auto stmt = std::make_shared<ExprAssignStmt>();
        stmt->target = expr;
        stmt->value = parseExpression();
        if (check(TokenType::Semicolon)) {
            advance();
        }
        return stmt;
    }

    if (check(TokenType::Semicolon)) {
        advance();
    }
    auto stmt = std::make_shared<ExprStmt>();
    stmt->expr = expr;
    return stmt;
}

StmtPtr Parser::parseReturn(std::string retype) {
    advance(); // 'ret'
    auto stmt = std::make_shared<ReturnStmt>();
    if (retype != "void") {
        if (!check(TokenType::Semicolon)) {
            stmt->value = parseExpression();
        }
    }
    else if (retype == "void") {
        reportError("ret cannot be used in a void function.");
    }
    else if (retype == "") {
        reportError("ret cannot be used outside a function.");
        advance();
    }
    else {
        reportError("Unknown return error.");
        advance();
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
    stmt->body = parseBlock("");
    return stmt;
}

StmtPtr Parser::parseElif() {
    advance(); // 'elif'
    auto stmt = std::make_shared<ElifStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock("");
    return stmt;
}

StmtPtr Parser::parseElse() {
    advance(); // 'else'
    auto stmt = std::make_shared<ElseStmt>();
    stmt->body = parseBlock("");
    return stmt;
}

StmtPtr Parser::parseWhile() {
    advance(); // 'while'
    auto stmt = std::make_shared<WhileStmt>();
    stmt->condition = parseExpression();
    stmt->body = parseBlock("");
    return stmt;
}

StmtPtr Parser::parseRepeat() {
    advance(); // repeat
    auto value = std::make_shared<RepeatCode>();
    value->value = parseExpression();
    value->body = parseBlock("");
    return value;
}

StmtPtr Parser::parseForever() {
    advance(); // forever
    auto stmt = std::make_shared<ForeverCode>();
    stmt->body = parseBlock("");
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
    stmt->body = parseBlock("");
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
StmtPtr Parser::parsePrintMac() {
    bool newline = check(TokenType::PrintMacLn);
    advance(); // 'print!' or 'println!'
    if (check(TokenType::Not)) advance();
    expect(TokenType::LParen, "(");

    auto stmt = std::make_shared<PrintMacCode>();
    stmt->newline = newline;
    if (!check(TokenType::RParen)) {
        stmt->value = parseMacExpression();
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
        // input("prompt" >> var), var can be `x` or a member path like `Info.name`
        advance(); // '>>'
        stmt->prompt = firstOperand;
        stmt->target = parsePostfix();
    }
    else if (std::dynamic_pointer_cast<NameExpr>(firstOperand) || std::dynamic_pointer_cast<MemberExpr>(firstOperand)) {
        // input(var) or input(Info.name)
        stmt->target = firstOperand;
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
        // inputstr("prompt" >> var), var can be `x` or a member path like `Info.name`
        advance(); // '>>'
        stmt->prompt = firstOperand; // any expression is fine as a prompt, not just string literals
        stmt->target = parsePostfix();
    }
    else if (std::dynamic_pointer_cast<NameExpr>(firstOperand) || std::dynamic_pointer_cast<MemberExpr>(firstOperand)) {
        // inputstr(var) or inputstr(Info.name)
        stmt->target = firstOperand;
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

    if (check(TokenType::Semicolon)) {
        advance();
    }
    return stmt;
}

std::vector<StmtPtr> Parser::parseCFBlock(std::string retype) {
    expect(TokenType::LBrace, "{");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::SClose) && !check(TokenType::EndOfFile)) {
        StmtPtr s = parseStatement(retype);
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
    fnd->returnType = fn.returnType;
    fnd->body = parseCFBlock(fn.returnType);
    fn.body = fnd->body;

    return fnd;
}

StmtPtr Parser::parseLambdaFn(std::string retype) {
    advance(); // 'lambda'
    Token nameTok = expect(TokenType::Identifier, "function name");
    expect(TokenType::LParen, "(");

    LambFuncDecl fn;
    auto fnd = std::make_shared<LambFuncDecl>();
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
    fnd->returnType = fn.returnType;
    fnd->body = parseCFBlock(fn.returnType);
    fn.body = fnd->body;

    return fnd;
}

StmtPtr Parser::parseStatement(std::string retype) {
    if (isTypeToken(peek().type, peek().text) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Ret)) return parseReturn(retype);
    if (check(TokenType::Lambda)) return parseLambdaFn(retype);
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
    if (check(TokenType::Repeat)) return parseRepeat();
    if (check(TokenType::Forever)) return parseForever();
    if (check(TokenType::PrintMac) || check(TokenType::PrintMacLn)) return parsePrintMac();

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

StmtPtr Parser::parseSStr() {
    if (isTypeToken(peek().type, peek().text) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Identifier)) return parseAssignOrExprStatement();
    if (check(TokenType::Print) || check(TokenType::PrintLine) ||
        check(TokenType::If) || check(TokenType::Elif) ||
        check(TokenType::Else) || check(TokenType::While) ||
        check(TokenType::For) || check(TokenType::Read) ||
        check(TokenType::ReadLine) || check(TokenType::Break) ||
        check(TokenType::Clear) || check(TokenType::Continue) ||
        check(TokenType::Ret) ||
        check(TokenType::Repeat) ||
        check(TokenType::Forever)) reportError("logic is not allowed in struct");

    reportError("unexpected token '" + peek().text + "'");
    recoverStatement();
    return nullptr;
}

StmtPtr Parser::parseSClass() {
    if (isTypeToken(peek().type, peek().text) || check(TokenType::List)) return parseVarDecl();
    if (check(TokenType::Fn)) return parseCFunction();
    if (check(TokenType::Print) || check(TokenType::PrintLine) ||
        check(TokenType::If) || check(TokenType::Elif) ||
        check(TokenType::Else) || check(TokenType::While) ||
        check(TokenType::For) || check(TokenType::Read) ||
        check(TokenType::ReadLine) || check(TokenType::Break) ||
        check(TokenType::Clear) || check(TokenType::Continue) ||
        check(TokenType::Ret) ||
        check(TokenType::Repeat) ||
        check(TokenType::Forever)) reportError("logic is not allowed in class");
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
    fn.body = parseBlock(fn.returnType);
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