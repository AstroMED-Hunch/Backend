#include "parser.hpp"
#include <print>
#include <stdexcept>
#include <memory>
#include <iostream>

namespace MLayout {
    CodeGroup* find_codeset_by_tag(const std::string& tag, Layout* layout) {
        for (auto& codeset : layout->code_groups) {
            if (codeset->tag == tag) {
                return codeset;
            }
        }
        return nullptr;
    }

    Interpreter get_interpreter_from_string(const std::string& str) {
        if (str == "code") {
            return Interpreter::INTERPRETER_CODE;
        } else if (str == "shelf") {
            return Interpreter::INTERPRETER_SHELF;
        } else if (str == "layout") {
            return Interpreter::INTERPRETER_LAYOUT;
        } else {
            return Interpreter::INTERPRETER_NONE;
        }
    }

    Variable get_symbol_value(TokenBase* token, Environment& env) {
        switch (token->get_type()) {
            case ML_Token::TOKEN_SYMBOL: {
                SymbolToken* sym_token = dynamic_cast<SymbolToken*>(token);
                if (env.variables.find(sym_token->value) != env.variables.end()) {
                    return env.variables[sym_token->value];
                } else {
                    throw std::runtime_error("Undefined variable: " + sym_token->value);
                }
            }
            case ML_Token::TOKEN_NUMBER: {
                NumberToken* num_token = dynamic_cast<NumberToken*>(token);
                Variable var;
                var.type = VariableType::VAR_INT;
                var.value.int_value = num_token->value;
                return var;
            }
            case ML_Token::TOKEN_STRING: {
                StringToken* str_token = dynamic_cast<StringToken*>(token);
                Variable var;
                var.type = VariableType::VAR_STRING;
                var.value.string_value = new std::string(str_token->value);
                return var;
            }
            default:
                throw std::runtime_error("Unexpected token");
        }
        
    }

    std::unique_ptr<Layout> parse_tokens(const std::vector<TokenBase*>& tokens) {
        auto iter = tokens.begin();
        Environment env;

        while (iter != tokens.end())
        {
            if ((*iter)->get_type() == ML_Token::TOKEN_SYMBOL) {
                SymbolToken* sym_token = dynamic_cast<SymbolToken*>(*iter);
                if (sym_token->value == "named") {
                    ++iter;
                    if ((*iter)->get_type() != ML_Token::TOKEN_SYMBOL) {
                        throw std::runtime_error("Expected symbol after 'named'");
                    }
                    SymbolToken* name_token = dynamic_cast<SymbolToken*>(*iter);
                    std::string name = name_token->value;
                    ++iter;
                    Variable value = get_symbol_value(*iter, env);
                    env.variables[name.c_str()] = value;
                    ++iter;
                }
                else if (sym_token->value == "create_layout") {
                    ++iter;
                    Variable layout_name = get_symbol_value(*iter, env);
                    Layout* layout = new Layout();
                    if (layout_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for layout name");
                    }
                    layout->name = *(layout_name.value.string_value);
                    env.layouts.push_back(layout);
                    ++iter;
                }
                else if (sym_token->value == "summon_layout" ) {
                    ++iter;
                    Variable layout_name = get_symbol_value(*iter, env);
                    if (layout_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for layout name");
                    }
                    std::string lname = *(layout_name.value.string_value);

                    Layout* found_layout = nullptr;
                    for (auto& lyt : env.layouts) {
                        if (lyt->name == lname) {
                            found_layout = lyt;
                            break;
                        }
                    }
                    if (found_layout == nullptr) {
                        throw std::runtime_error("Layout not found: " + lname);
                    }
                    else {
                        env.current_layout = found_layout;
                    }
                    ++iter;
                }
                else if (sym_token->value == "create_codeset") {
                    ++iter;
                    Variable codeset_name = get_symbol_value(*iter, env);
                    CodeGroup* codeset = new CodeGroup();
                    if (codeset_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for codeset name");
                    }
                    codeset->tag = *(codeset_name.value.string_value);
                    env.current_layout->code_groups.push_back(codeset);
                    ++iter;
                }
                else if (sym_token->value == "summon_codeset" ) {
                    ++iter;
                    Variable codeset_name = get_symbol_value(*iter, env);
                    if (codeset_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for codeset name");
                    }
                    std::string cname = *(codeset_name.value.string_value);

                    CodeGroup* found_codeset = nullptr;
                    for (auto& grp : env.current_layout->code_groups) {
                        if (grp->tag == cname) {
                            found_codeset = grp;
                            break;
                        }
                    }
                    if (found_codeset == nullptr) {
                        throw std::runtime_error("Codeset not found: " + cname);
                    }
                    else {
                        env.current_code_group = found_codeset;
                    }
                    ++iter;
                }
                else if (sym_token->value == "interpreter_set") {
                    ++iter;
                    if (env.current_code_group == nullptr) {
                        throw std::runtime_error("No current codeset to set interpreter for");
                    }
                    Variable interpreter_name = get_symbol_value(*iter, env);
                    if (interpreter_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for interpreter name");
                    }
                    env.current_code_group->interpreter = get_interpreter_from_string(*(interpreter_name.value.string_value));
                    ++iter;
                }
                else if (sym_token->value == "push_code") {
                    ++iter;
                    Variable marker_id = get_symbol_value(*iter, env);
                    if (marker_id.type != VariableType::VAR_INT) {
                        throw std::runtime_error("Expected integer for marker ID");
                    }
                    if (env.current_code_group == nullptr) {
                        throw std::runtime_error("No current codeset to push code to");
                    }
                    std::shared_ptr<ArucoMarker> marker = std::make_shared<ArucoMarker>();
                    marker->set_code_id(marker_id.value.int_value);
                    env.current_code_group->markers.push_back(marker);
                    ++iter;
                }
                else if (sym_token->value == "bind") {
                    ++iter;
                    Variable tag_name = get_symbol_value(*iter, env);
                    if (tag_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for tag name");
                    }
                    if (env.current_layout == nullptr) {
                        throw std::runtime_error("No current layout to bind to");
                    }
                    std::string tname = *(tag_name.value.string_value);

                    CodeGroup* bound_codeset = find_codeset_by_tag(tname, env.current_layout);
                    if (bound_codeset) {
                        bound_codeset->bound_to.push_back(env.current_layout);
                    }
                    ++iter;

                    if ((*iter)->get_type() != ML_Token::TOKEN_SYMBOL) {
                        throw std::runtime_error("Expected symbol after bind");
                    }
                    SymbolToken* bind_sym_token = dynamic_cast<SymbolToken*>(*iter);
                    if (bind_sym_token->value != "to") {
                        throw std::runtime_error("Expected 'to' after bind");
                    }
                    ++iter;
                    Variable to_codeset_name = get_symbol_value(*iter, env);
                    if (to_codeset_name.type != VariableType::VAR_STRING) {
                        throw std::runtime_error("Expected string for codeset name in bind");
                    }
                    std::string to_cname = *(to_codeset_name.value.string_value);
                    CodeGroup* to_codeset = find_codeset_by_tag(to_cname, env.current_layout);
                    if (to_codeset == nullptr) {
                        throw std::runtime_error("Codeset not found for bind: " + to_cname);
                    }
                    to_codeset->bound_to.push_back(env.current_layout);
                    std::print("Bound codeset {} to layout {}\n", to_codeset->tag, env.current_layout->name);
                    ++iter;
                }
                else {
                    throw std::runtime_error("Unknown symbol: " + sym_token->value);
                }

            }
            else {
                throw std::runtime_error("Expected expression, instead got:" + std::to_string(static_cast<int>((*iter)->get_type())) + " at: " + std::to_string(std::distance(tokens.begin(), iter)));
            }
        }

        return std::make_unique<Layout>(*(env.current_layout));
    }

    std::unique_ptr<Layout> parse_file(const std::string &filepath) {
        std::vector<TokenBase*> tokens = lex_file(filepath);
        return parse_tokens(tokens);
    }
}