#include "mlayout/Lexer.hpp"
#include <fstream>

namespace MLayout {
    std::vector<TokenBase*> lex_string(const std::string& input) {
        std::vector<TokenBase*> tokens;
        size_t i = 0;

        while (i < input.size()) {
            if (std::isspace(static_cast<unsigned char>(input[i]))) {
                ++i;
            }
            else if (input[i] == '#') {
                while (i < input.size() && input[i] != '\n') {
                    ++i;
                }
            }
            else if (std::isdigit(static_cast<unsigned char>(input[i]))) {
                int number = 0;
                while (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) {
                    number = number * 10 + (input[i] - '0');
                    ++i;
                }
                tokens.push_back(new NumberToken(number));
            }
            else if (std::isalpha(static_cast<unsigned char>(input[i]))) {
                std::string symbol;
                while (i < input.size() && (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '_')) {
                    symbol += input[i];
                    ++i;
                }
                tokens.push_back(new SymbolToken(symbol));
            }
            else if (input[i] == '\"') {
                std::string str_value;
                ++i;
                while (i < input.size() && input[i] != '\"') {
                    str_value += input[i];
                    ++i;
                }
                if (i < input.size()) {
                    ++i; 
                }
                tokens.push_back(new StringToken(str_value));
            }
            else {
                ++i;
            }
        }

        return tokens;
    }

    std::vector<TokenBase*> lex_file(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            try {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return lex_string(content);
            } catch (std::exception& e) {
                std::printf("Error reading file: %s\n", filename.c_str());
                std::printf("Exception: %s\n", e.what());
                return std::vector<TokenBase*>();
            }

        } else {
            return std::vector<TokenBase*>();
        }
    }
}
