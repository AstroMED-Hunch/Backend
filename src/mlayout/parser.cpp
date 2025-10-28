#include "parser.hpp"
#include <stdexcept>

namespace MLayout {
    SymbolToken* get_symbol_token(TokenBase* token) {
        
    }

    std::unique_ptr<Layout> parse_tokens(const std::vector<TokenBase*>& tokens) {
        Layout* layout = new Layout();
        auto iter = tokens.begin();

        while (iter != tokens.end())
        {
            if ((*iter)->get_type() == ML_Token::TOKEN_SYMBOL) {

            }
            else {
                throw std::runtime_error("Expected expression");
            }
        }

        return std::unique_ptr<Layout>(layout);
    }

    std::unique_ptr<Layout> parse_file(const std::string &filepath) {
        std::vector<TokenBase*> tokens = lex_file(filepath);
        return parse_tokens(tokens);
    }
}