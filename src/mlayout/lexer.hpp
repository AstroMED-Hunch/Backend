#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace MLayout
{

    enum class ML_Token
    {
        TOKEN_SYMBOL,
        TOKEN_NUMBER,
        TOKEN_STRING,
        TOKEN_WHITESPACE,
    };

    class TokenBase
    {
    public:
        virtual ML_Token get_type() const = 0;
    };

    template <typename T, ML_Token tokenType>
    class Token : public TokenBase
    {
    public:
        ML_Token get_type() const override
        {
            return tokenType;
        }
        T value;
    };

    // TOKEN IMPLEMENTATIONS

    class SymbolToken : public Token<std::string, ML_Token::TOKEN_SYMBOL>
    {
    public:
        SymbolToken(const std::string &val)
        {
            value = val;
        }
    };

    class NumberToken : public Token<int, ML_Token::TOKEN_NUMBER>
    {
    public:
        NumberToken(int val)
        {
            value = val;
        }
    };

    class StringToken : public Token<std::string, ML_Token::TOKEN_STRING>
    {
    public:
        StringToken(const std::string &val)
        {
            value = val;
        }
    };

    class WhitespaceToken : public Token<int, ML_Token::TOKEN_WHITESPACE>
    {
    public:
        WhitespaceToken()
        {
            value = 0;
        }
    };

    std::vector<TokenBase*> lex_string(const std::string &input);
    std::vector<TokenBase*> lex_file(const std::string &filename);

}