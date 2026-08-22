#include "lexer.hpp"
#include <cctype>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    // Syntax
    {"def", TokenType::Fn}, {"ret", TokenType::Ret}, {"lambda", TokenType::Lambda},
    {"class", TokenType::Class}, {"public", TokenType::Public}, {"private", TokenType::Private},
    {"autouse", TokenType::AutoUse}, {"use", TokenType::Use}, {"as", TokenType::UseAs},
    {"module", TokenType::Module}, {"^", TokenType::Arrow_Up},
    {"struct", TokenType::Struct},
    {"if", TokenType::If}, {"elif", TokenType::Elif}, {"else", TokenType::Else},
    {"while", TokenType::While}, {"for", TokenType::For}, {"in", TokenType::In},
    {"repeat", TokenType::Repeat}, {"forever", TokenType::Forever},
    {"import", TokenType::Import},
    {"print", TokenType::Print}, {"println", TokenType::PrintLine},
    {"print!", TokenType::PrintMac}, {"println!", TokenType::PrintMacLn},
    {"read", TokenType::Read}, {"readln", TokenType::ReadLine},
    {"continue", TokenType::Continue}, {"break", TokenType::Break},
    {"clear", TokenType::Clear}, {"ctype", TokenType::CType},

    // Operators
    {"and", TokenType::AndAnd},
    {"or", TokenType::OrOr},

    // Data Types
    {"List", TokenType::List},
    {"int", TokenType::TypeInt},
    {"str", TokenType::TypeStr},
    {"string", TokenType::TypeString},
    {"float", TokenType::TypeFloat},
    {"double", TokenType::TypeDouble},
    {"byte", TokenType::TypeByte},
    {"char", TokenType::TypeChar},
    {"bool", TokenType::TypeBool},
    {"void", TokenType::TypeVoid},
    {"auto", TokenType::TypeAuto},
    {"long", TokenType::TypeLong},
    {"longer", TokenType::TypeLonger},
    {"uint8", TokenType::TypeInt8},
    {"uint16", TokenType::TypeInt16},
    {"uint32", TokenType::TypeInt32},
    {"uint64", TokenType::TypeInt64},
    {"float16", TokenType::TypeFloat16},
    {"float32", TokenType::TypeFloat32},
    {"float64", TokenType::TypeFloat64},
    {"float128", TokenType::TypeFloat128},
    {"frac", TokenType::TypeFrac},
};

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> tokens;
    size_t i = 0;
    int line = 1;
    size_t n = src.size();
    size_t tokenStart = 0; // byte offset where the token currently being scanned began

    auto push = [&](TokenType t, const std::string& text) {
        tokens.push_back({ t, text, line, tokenStart });
        };

    while (i < n) {
        char c = src[i];

        if (c == '\n') { line++; i++; continue; }
        if (std::isspace(static_cast<unsigned char>(c))) { i++; continue; }

        tokenStart = i; // every push() below belongs to the token starting here

        // raw string block: %" ... "%
        if (c == '%' && i + 1 < n && src[i + 1] == '"') {
            i += 2; // consume '%"'
            std::string raw;
            bool closed = false;
            while (i < n) {
                if (src[i] == '"' && i + 1 < n && src[i + 1] == '%') {
                    i += 2; // consume '"%'
                    closed = true;
                    break;
                }
                if (src[i] == '\n') line++;
                raw += src[i];
                i++;
            }
            if (!closed) {
                push(TokenType::Invalid, "unterminated raw string block %\"...\"%");
                continue;
            }
            push(TokenType::LnQuote, raw); // whole raw body, as-is, as ONE token
            continue;
        }

        // string literal
        if (c == '"') {
            std::string s = "\"";
            i++;
            while (i < n && src[i] != '"') {
                if (src[i] == '\\' && i + 1 < n) { s += src[i]; s += src[i + 1]; i += 2; continue; }
                if (src[i] == '\n') break; // don't let a string swallow the rest of the file
                s += src[i];
                i++;
            }
            if (i >= n || src[i] != '"') {
                push(TokenType::Invalid, "unterminated string literal " + s);
                continue; // don't advance past EOF/newline
            }
            s += '"';
            i++; // closing quote
            push(TokenType::String, s);
            continue;
        }

        // char literal
        if (c == '\'') {
            std::string s = "";
            i++;
            while (i < n && src[i] != '\'') {
                if (src[i] == '\\' && i + 1 < n) { s += src[i]; s += src[i + 1]; i += 2; continue; }
                if (src[i] == '\n') break; // don't let a char swallow the rest of the file
                s += src[i];
                i++;
            }
            if (i >= n || src[i] != '\'') {
                push(TokenType::Invalid, "unterminated char literal " + s);
                continue; // don't advance past EOF/newline
            }
            i++; // closing quote
            push(TokenType::Char, s); // treat char literals as strings for simplicity
            continue;
        }

        // number literal (int or float)
        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (i < n && (std::isdigit(static_cast<unsigned char>(src[i])) || src[i] == '.')) {
                num += src[i];
                i++;
            }
            push(TokenType::Number, num);
            continue;
        }

        // identifier / keyword
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string word;
            while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) {
                word += src[i];
                i++;
            }
            if (i < n && src[i] == '!') {
                std::string bangWord = word + "!";
                auto bangIt = keywords.find(bangWord);
                if (bangIt != keywords.end()) {
                    i++; // consume '!'
                    push(bangIt->second, bangWord);
                    continue;
                }
            }
            auto it = keywords.find(word);
            if (it != keywords.end()) push(it->second, word);
            else push(TokenType::Identifier, word);
            continue;
        }

        // two-character operators
        if (i + 1 < n) {
            std::string two = src.substr(i, 2);
            if (two == "==") { push(TokenType::Eq, two); i += 2; continue; }
            if (two == "!=") { push(TokenType::Neq, two); i += 2; continue; }
            if (two == "<=") { push(TokenType::Le, two); i += 2; continue; }
            if (two == ">=") { push(TokenType::Ge, two); i += 2; continue; }
            if (two == "&&") { push(TokenType::AndAnd, two); i += 2; continue; }
            if (two == "||") { push(TokenType::OrOr, two); i += 2; continue; }
            if (two == "<<") { push(TokenType::Shl, two); i += 2; continue; }
            if (two == ">>") { push(TokenType::Shr, two); i += 2; continue; }
            if (two == "::") { push(TokenType::DoubleColon, two); i += 2; continue; }
            if (two == "++") { push(TokenType::PlusPlus, two); i += 2; continue; }
            if (two == "--") { push(TokenType::MinusMinus, two); i += 2; continue; }
            if (two == "+=") { push(TokenType::AssignAdd, two); i += 2; continue; }
            if (two == "-=") { push(TokenType::AssignMinus, two); i += 2; continue; }
            if (two == "*=") { push(TokenType::AssignMulti, two); i += 2; continue; }
            if (two == "/=") { push(TokenType::AssignSlash, two); i += 2; continue; }
            if (two == "};") { push(TokenType::SClose, two); i += 2; continue; }
        }

        // single-character tokens
        switch (c) {
        case '(': push(TokenType::LParen, "("); break;
        case ')': push(TokenType::RParen, ")"); break;
        case '{': push(TokenType::LBrace, "{"); break;
        case '}': push(TokenType::RBrace, "}"); break;
        case '[': push(TokenType::LBracket, "["); break;
        case ']': push(TokenType::RBracket, "]"); break;
        case ',': push(TokenType::Comma, ","); break;
        case ';': push(TokenType::Semicolon, ";"); break;
        case ':': push(TokenType::Colon, ":"); break;
        case '@': push(TokenType::At, "@"); break;
        case '=': push(TokenType::Assign, "="); break;
        case '<': push(TokenType::Lt, "<"); break;
        case '>': push(TokenType::Gt, ">"); break;
        case '+': push(TokenType::Plus, "+"); break;
        case '-': push(TokenType::Minus, "-"); break;
        case '*': push(TokenType::Star, "*"); break;
        case '/': push(TokenType::Slash, "/"); break;
        case '.': push(TokenType::Dot, "."); break;
        case '!': push(TokenType::Not, "!"); break;
        case '^': push(TokenType::Arrow_Up, "^"); break;
        default:
            push(TokenType::Invalid, std::string(1, c));
            break;
        }
        i++;
    }

    tokenStart = n;
    push(TokenType::EndOfFile, "");
    return tokens;
}