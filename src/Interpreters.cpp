#include "Interpreters.hpp"
#include "mlayout/Parser.hpp"

namespace interpreters {
    std::unordered_map<MLayout::Interpreter, std::function<void(MLayout::Layout&, MLayout::CodeGroup*)>> interpreter_registry = {
        {MLayout::Interpreter::INTERPRETER_BOX, [](MLayout::Layout& layout, MLayout::CodeGroup* code_group) {

        }},
        {MLayout::Interpreter::INTERPRETER_SHELF, [](MLayout::Layout& layout, MLayout::CodeGroup* code_group) {

        }},
        {MLayout::Interpreter::INTERPRETER_LAYOUT, [](MLayout::Layout& layout, MLayout::CodeGroup* code_group) {
            for (const auto& bound_codeset : code_group->bound_to) {
                if (bound_codeset->interpreter == MLayout::Interpreter::INTERPRETER_SHELF) {
                    bound_codeset->interpret(layout);

                }
                else {
                    throw std::runtime_error("layout interpreter can only bind to shelf interpreters");
                }
            }
        }}
    };
}