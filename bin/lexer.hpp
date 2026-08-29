#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

#ifdef TokenType
  #undef TokenType
#endif

enum class TokenType {
    // literals / names
    Identifier, Number, String, Char,
    // keywords
    Module, Arrow_Up,
    Class, Public, Private,
    AutoUse, Use, UseAs, UseClass,
    nUse,
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
    Clear, CType,
    Try, Except,
    // built-in types
    TypeInt, TypeString, TypeFloat, TypeDouble, TypeByte,
    TypeChar, TypeBool, TypeVoid, TypeAuto, TypeFrac,
    TypeLong, TypeInt8, TypeInt16, TypeInt32, TypeInt64,
    TypeLonger, TypeFloat16, TypeFloat32, TypeFloat64,
    TypeFloat128, TypeStr, TypeFILE,
    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, DoubleColon, At, Dot, SClose,
    Not, LnQuote, LnClose, Dollar, And,
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