#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include "vec.hpp"

class CodeGroup;

class ArucoMarker {
    public:
        ArucoMarker();

        void set_code_id(int id);
        int get_code_id() const;

        std::shared_ptr<CodeGroup> get_code_group() const;
        
        Vec2 position;

    protected:
        int code_id = -1;
        std::weak_ptr<CodeGroup> code_group;

        mutable std::recursive_mutex mutex;
};

class Markers {
    public:
        static std::vector<std::shared_ptr<ArucoMarker>> registered_markers;
};