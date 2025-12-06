#include "Face_Recognition.hpp"

#include <iostream>

std::string Face_Recognition::get_module_name() const {
    return "Face Recognition";
}

void Face_Recognition::initialize() {
    std::cout << "Face Recognition module initialized." << std::endl;
}

void Face_Recognition::run() {

}

void Face_Recognition::shutdown() {
}
