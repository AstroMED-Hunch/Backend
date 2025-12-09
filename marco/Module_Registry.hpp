#pragma once
#include <functional>
#include <string>

class Module;
using ModuleCreateFunc = std::function<Module*()>;

class Module_Registry {
public:
    static void register_module(const std::string& module_name, ModuleCreateFunc create_func);
    static Module* create_module(const std::string& module_name);
private:
    Module_Registry() = default;
    static std::unordered_map<std::string, ModuleCreateFunc> registry;
};

struct RegisterModule {
    RegisterModule(ModuleCreateFunc, const std::string&);
};

#define MAKE_MODULE(MODULE_CLASS, MODULE_NAME) \
    namespace { \
        Module* create_##MODULE_CLASS() { \
            return new MODULE_CLASS(); \
        } \
        static RegisterModule register_##MODULE_CLASS(create_##MODULE_CLASS, MODULE_NAME); \
    }
