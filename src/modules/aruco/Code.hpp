#pragma once
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../Vec.hpp"

class CodeGroup;

class ArucoMarker {
    public:
        static std::unordered_map<int, ArucoMarker*> registered_markers;
        ArucoMarker(int new_id);
        ~ArucoMarker();

        void set_code_id(int id);
        int get_code_id() const;

        std::shared_ptr<CodeGroup> get_code_group() const;
        
        Vec2 position;
        bool is_visible = false;

    protected:
        int code_id = -1;
        std::weak_ptr<CodeGroup> code_group;

        mutable std::recursive_mutex mutex;
};

class Markers {
    public:
        static std::vector<std::shared_ptr<ArucoMarker>> registered_markers;
};