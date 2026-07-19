#include "parser.hpp"
#include <iostream>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

// ---------- Core cursor / matching infra ----------

const Token& Parser::peek() const {
    return tokens[current];
}

Token Parser::advance() {
    Token t = tokens[current];
    if (current + 1 < tokens.size()) {
        current++;
    }
    return t;
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& what) {
    if (check(type)) {
        return advance();
    }
    reportError("expected " + what + " but got '" + peek().text + "'");
    return peek(); // let the caller keep going against the same token
}

void Parser::reportError(const std::string& message) {
    errors.push_back("Syntax Error on line " + std::to_string(peek().line) + ": " + message);
}

void Parser::recoverStatement() {
    while (!check(TokenType::Semicolon) &&
           !check(TokenType::RBrace) &&
           !check(TokenType::EndOfFile)) {
        advance();
    }
    if (check(TokenType::Semicolon)) {
        advance();
    }
}

// ---------- Top-level driver ----------

Program Parser::parse() {
    Program program;

    while (!check(TokenType::EndOfFile)) {
        size_t startPos = current;

        if (check(TokenType::At)) {
            parseImport(program);
        } else if (check(TokenType::Fn)) {
            program.functions.push_back(parseFunction());
        } else {
            reportError("unexpected top-level token '" + peek().text + "'");
            recoverStatement();
        }

        // Safety net
        if (current == startPos) {
            advance();
        }
    }

    if (!errors.empty()) {
        for (const std::string& e : errors) {
            std::cerr << e << '\n';
        }
        std::cerr << errors.size() << " error(s) found.\n";
        std::exit(EXIT_FAILURE);
    }

    return program;
}