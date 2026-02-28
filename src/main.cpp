#include <opencv2/objdetect/aruco_dictionary.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "Module.hpp"
#include "Module_Registry.hpp"
#include "ShelfDatabase.hpp"
#include "mlayout/Parser.hpp"

#include "modules/aruco/Code.hpp"

constexpr int DEFAULT_CAM_INDEX = 0;
MLayout::Layout *layout_global = nullptr;

MLayout::Layout& get_global_layout() {
    return *layout_global;
}

int main(int argc, char** argv) {
    int cam_index = DEFAULT_CAM_INDEX;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--camera" && i + 1 < argc) {
            cam_index = std::stoi(argv[++i]);
            std::cout << "Camera index set to: " << cam_index << std::endl;
        }
    }

    std::unique_ptr<MLayout::Layout> layout_unique = MLayout::parse_file("../extern/shelf_testing.mlayout");
    std::shared_ptr<MLayout::Layout> layout = std::move(layout_unique);
    layout_global = layout.get();

    for (const auto& code_group : layout->code_groups) {
        std::cout << "Code Group: " << code_group->tag << std::endl;

        if (code_group->interpreter == MLayout::Interpreter::INTERPRETER_SHELF) {
            ShelfEntry* shelf_entry = new ShelfEntry(code_group->tag);
            ShelfDatabase::push_shelf_entry(*shelf_entry);
            std::cout << "Added shelf entry: " << code_group->tag << std::endl;
        }

        std::cout << code_group->markers.size() << std::endl;
        for (const auto& code : code_group->markers) {
            std::cout << "Code: " << code->get_code_id() << std::endl;
        }

        for (const auto& layout_idx : code_group->bound_to) {
            std::cout << "Bound to layout: " << layout_idx->tag.c_str() << std::endl;
        }

        for (const auto& person : layout->people) {
            std::cout << "Person: " << person.first << " with face: " << person.second << std::endl;
        }
    }

    std::vector<Module*> modules;
    modules.reserve(layout->module_load_requests.size());

    for (const auto& module_name : layout->module_load_requests) {
        std::cout << "Attempting to load module: " << module_name << std::endl;
        if (Module* module = Module_Registry::create_module(module_name)) {
            module->layout = layout;
            module->initialize();
            modules.push_back(module);
            std::cout << "Loaded module: " << module_name << std::endl;
        } else {
            std::cerr << "Failed to load module: " << module_name << std::endl;
        }
    }

    modules.shrink_to_fit();

    const std::string camera_index_str = layout->get_config_value("camera_index");
    const int camera_index = std::stoi(camera_index_str);
    cv::VideoCapture cap(camera_index);

    while (true) {
        cv::Mat frame;
        cap >> frame;
        for (const auto& module : modules) {
            module->run(frame);
        }

        cv::imshow("Hunch Medical", frame);

        if (cv::waitKey(30) >= 0) break;
    }

    return 0;
}