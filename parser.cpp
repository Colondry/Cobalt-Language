#include "parser.hpp"
#include <iostream>
#include <algorithm>

Parser::Parser(std::vector<Token> tokens, std::string source) : tokens(std::move(tokens)), source(std::move(source)) {}

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
    const Token& tok = peek();

    if (source.empty()) {
        // No source text available -- fall back to a plain line-only
        // message rather than doing string ops on an empty source.
        errors.push_back("Syntax Error on line " + std::to_string(tok.line) + ": " + message);
        return;
    }

    size_t pos = std::min(tok.pos, source.size());

    size_t lineStart = (pos == 0) ? 0 : source.rfind('\n', pos - 1);
    lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;
    size_t lineEnd = source.find('\n', pos);
    if (lineEnd == std::string::npos) lineEnd = source.size();

    std::string lineText = source.substr(lineStart, lineEnd - lineStart);
    if (!lineText.empty() && lineText.back() == '\r') lineText.pop_back(); // CRLF sources

    int column = static_cast<int>(pos - lineStart) + 1; // 1-based

    // An Invalid token's .text is a diagnostic message (e.g. "unterminated
    // string literal ..."), not source text -- point at just the one bad
    // character rather than underlining that whole message.
    size_t caretLen = (tok.type == TokenType::Invalid) ? 1 : tok.text.size();
    caretLen = std::max<size_t>(1, caretLen);
    // Don't let the caret run past the end of the (possibly short) line.
    size_t maxCaretLen = (static_cast<size_t>(column - 1) < lineText.size())
        ? lineText.size() - (column - 1)
        : 1;
    caretLen = std::min(caretLen, std::max<size_t>(1, maxCaretLen));

    std::string pointerLine = std::string(column - 1, ' ') + std::string(caretLen, '^');

    errors.push_back(
        "Syntax Error on line " + std::to_string(tok.line) + ", column " + std::to_string(column) +
        ": " + message + "\n" +
        "    " + lineText + "\n" +
        "    " + pointerLine
    );
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
    program = Program();

    while (!check(TokenType::EndOfFile)) {
        size_t startPos = current;

        if (check(TokenType::At)) {
            parseImport(program);
        }
        else if (check(TokenType::Fn)) {
            program.functions.push_back(parseFunction());
        }
        else if (check(TokenType::Class)) {
            program.classes.push_back(parseClasses());
        }
        else if (check(TokenType::RBrace) || check(TokenType::SClose)) {
        }
        else if (check(TokenType::Struct)) {
            program.struc.push_back(parseStruct());
        }
        else {
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
