#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

enum class TokenType {
    // literals / names
    Identifier, Number, String, Char,
    // keywords
    Class, Public, Private,
    Struct,
    Fn, Ret,
    If, Elif, Else,
    While, For, Repeat, Forever,
    In,
    Import,
    List,
    Print, PrintLine, Read, ReadLine,
    Continue, Break,
    Clear,
    // built-in types
    TypeInt, TypeString, TypeFloat, TypeDouble, TypeByte, TypeChar, TypeBool, TypeVoid, TypeAuto,
    TypeLong,
    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, DoubleColon, At, Dot, SClose,
    // operators
    Assign, Eq, Neq, Lt, Gt, Le, Ge, AndAnd, OrOr,
    Plus, Minus, Star, Slash, Shl, Shr,
    PlusPlus, MinusMinus,
    AssignAdd, AssignMinus, AssignMulti, AssignSlash,
    // unrecognized character
    Invalid,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    size_t pos = 0; // byte offset of this token's first character in the source
};

std::vector<Token> tokenize(const std::string& source);

#endif