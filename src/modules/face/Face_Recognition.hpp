#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"


class Face_Recognition final : public Module {
public:
    ~Face_Recognition() override = default;
    [[nodiscard]] std::string get_module_name() const override;

    void initialize() override;
    void run() override;
    void shutdown() override;
};

MAKE_MODULE(Face_Recognition, "face_recognition");
