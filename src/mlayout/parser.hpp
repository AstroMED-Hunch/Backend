#pragma once
#include "lexer.hpp"
#include "code.hpp"

namespace MLayout {
    enum class Interpreter {
        INTERPRETER_NONE,
        INTERPRETER_CODE,
        INTERPRETER_SHELF,
    };
    struct CodeGroup {
        std::vector<std::shared_ptr<ArucoMarker>> markers;
        Interpreter interpreter = Interpreter::INTERPRETER_NONE;
        std::string tag;
    };
    struct Layout {
        std::vector<std::shared_ptr<CodeGroup>> code_groups;
        std::string name;
    };

    std::unique_ptr<Layout> parse_tokens(const std::vector<TokenBase*>& tokens);

    std::unique_ptr<Layout> parse_file(const std::string &filepath);
}