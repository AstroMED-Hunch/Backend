#include "mlayout/lexer.hpp"
#include "lexertk.hpp"

namespace MLayout {
    void lex_string(const std::string& input) {
        lexertk::generator Generator;
        if (Generator.process(input)) {
            Generator.begin();
            while (!Generator.finished()) {
                lexertk::token& tok = Generator.next_token();

                std::printf("Token Type: %s\n", tok.to_str(tok.type).c_str());
            }
        } else {
            std::fprintf(stderr, "Lexing error occurred.\n");
        }
    }
}