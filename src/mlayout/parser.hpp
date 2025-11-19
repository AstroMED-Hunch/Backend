#pragma once
#include "lexer.hpp"
#include "code.hpp"
#include <memory>
#include <unordered_map>
#include "interpreters.hpp"
#include "vec.hpp"

namespace MLayout {
    struct Layout; // forward declaration

    struct CodeGroup : public std::enable_shared_from_this<CodeGroup> {
        std::vector<std::shared_ptr<ArucoMarker>> markers;
        Interpreter interpreter = Interpreter::INTERPRETER_NONE;
        std::string tag;
        std::vector<CodeGroup*> bound_to;

        void interpret(Layout& layout);

        Vec3 corner_positions_box[2] = {Vec3(0,0,0), Vec3(0,0,0)};
        
    };
    struct Layout : public std::enable_shared_from_this<Layout> {
        std::vector<CodeGroup*> code_groups;
        std::string name;
    };

    Interpreter get_interpreter_from_string(const std::string& str);

    enum class VariableType {
        VAR_INT,
        VAR_FLOAT,
        VAR_STRING,
    };
    union Var {
        int int_value;
        float float_value;
        std::string* string_value;
    };

    struct Variable {
        VariableType type;
        Var value;
    };

    class Environment {
        public:
            std::unordered_map<std::string, Variable> variables;
            std::vector<Layout*> layouts;
            CodeGroup* current_code_group = nullptr;
            Layout* current_layout = nullptr;
    };

    std::unique_ptr<Layout> parse_tokens(const std::vector<TokenBase*>& tokens);

    std::unique_ptr<Layout> parse_file(const std::string &filepath);
}