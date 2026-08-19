#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

enum class TokenType {
    // literals / names
    Identifier, Number, String, Char,
    // keywords
    Module, Arrow_Up,
    Class, Public, Private,
    AutoUse, Use, UseAs, UseClass,
    Struct,
    Fn, Ret, Lambda,
    If, Elif, Else,
    While, For, Repeat, Forever,
    In,
    Import,
    List,
    Print, PrintLine, Read, ReadLine,
    PrintMac, PrintMacLn,
    Continue, Break,
    Clear,
    // built-in types
    TypeInt, TypeString, TypeFloat, TypeDouble, TypeByte,
    TypeChar, TypeBool, TypeVoid, TypeAuto, TypeFrac,
    TypeLong, TypeInt8, TypeInt16, TypeInt32, TypeInt64,
    TypeLonger,
    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, DoubleColon, At, Dot, SClose,
    Not,
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