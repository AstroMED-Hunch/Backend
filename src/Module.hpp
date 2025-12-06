#pragma once

#include <string>
#include "mlayout/Parser.hpp"

class Module {
public:
    MLayout::Layout* layout = nullptr;

    virtual ~Module() = default;
    [[nodiscard]] virtual std::string get_module_name() const;

    virtual void initialize() = 0;
    virtual void run() = 0;
    virtual void shutdown() = 0;

};