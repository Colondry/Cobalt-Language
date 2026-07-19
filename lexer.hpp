#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

enum class TokenType {
    // literals / names
    Identifier, Number, String,
    // keywords
    Fn, Ret, If, While, For, In, Import, List, Print, PrintLine, Input,
    // built-in types
    TypeInt, TypeString, TypeFloat, TypeDouble, TypeByte, TypeChar, TypeBool, TypeVoid,
    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, DoubleColon, At,
    // operators
    Assign, Eq, Neq, Lt, Gt, Le, Ge, AndAnd, OrOr,
    Plus, Minus, Star, Slash, Shl, Shr, PlusPlus,
    // unrecognized character
    Invalid,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    int line;
};

std::vector<Token> tokenize(const std::string& source);

#endif