#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace MLayout {
    struct Layout;

    enum class Interpreter {
        INTERPRETER_NONE,
        INTERPRETER_BOX,
        INTERPRETER_SHELF,
        INTERPRETER_LAYOUT
    };

    struct CodeGroup;
}

namespace interpreters {
    extern std::unordered_map<MLayout::Interpreter, std::function<void(MLayout::Layout&, MLayout::CodeGroup*)>> interpreter_registry;
}