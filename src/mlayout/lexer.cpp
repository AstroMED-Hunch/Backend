#include "mlayout/lexer.hpp"
#include <fstream>

namespace MLayout {
    std::vector<TokenBase*> lex_string(const std::string& input) {
        std::vector<TokenBase*> tokens;
        std::string::const_iterator str_iterator = input.begin();

        while (str_iterator != input.end()) {
            if (std::isspace(*str_iterator)) {
                tokens.push_back(new WhitespaceToken());
            }
            else if (std::isdigit(*str_iterator)) {
                int number = 0;
                while (str_iterator != input.end() && std::isdigit(*str_iterator)) {
                    number = number * 10 + (*str_iterator - '0');
                    ++str_iterator;
                }
                tokens.push_back(new NumberToken(number));
                continue;
            }
            else if (std::isalpha(*str_iterator)) {
                std::string symbol;
                while (str_iterator != input.end() && (std::isalnum(*str_iterator) || *str_iterator == '_')) {
                    symbol += *str_iterator;
                    ++str_iterator;
                }
                tokens.push_back(new SymbolToken(symbol));
                continue;
            }
            else if (*str_iterator == '\"') {
                std::string str_value;
                ++str_iterator;
                while (str_iterator != input.end() && *str_iterator != '\"') {
                    str_value += *str_iterator;
                    ++str_iterator;
                }
                ++str_iterator;
                tokens.push_back(new StringToken(str_value));
                continue;
            }
            ++str_iterator;
        }

        return tokens;
    }

    std::vector<TokenBase*> lex_file(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return lex_string(content);
        } else {
            throw std::runtime_error("Failed to open layout script file at: " + filename);
            
        }
    }
}