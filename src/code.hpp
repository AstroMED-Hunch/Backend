#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include "vec.hpp"

class CodeGroup;

class ArucoMarker : public std::enable_shared_from_this<ArucoMarker> {
    public:
        ArucoMarker(std::weak_ptr<CodeGroup> group);

        void set_code_id(int id);
        int get_code_id() const;
        
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