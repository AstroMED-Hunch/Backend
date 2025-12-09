//
// Created by user on 12/6/25.
//

#include "Module_Registry.hpp"

#include <utility>
#include "Module.hpp"

std::unordered_map<std::string, ModuleCreateFunc> Module_Registry::registry;

void Module_Registry::register_module(const std::string& module_name, ModuleCreateFunc create_func) {
    registry[module_name] = std::move(create_func);
}

Module* Module_Registry::create_module(const std::string& module_name) {
    auto it = registry.find(module_name);
    if (it != registry.end()) {
        return it->second();
    }
    return nullptr;
}

RegisterModule::RegisterModule(ModuleCreateFunc create_func, const std::string& module_name) {
    Module_Registry::register_module(module_name, std::move(create_func));
}