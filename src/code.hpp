#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <memory>

class ArucoMarker : public std::enable_shared_from_this<ArucoMarker> {
    public:
        ArucoMarker();
        float x_start = 0.0f, y_start = 0.0f;
        float x_end = 0.0f, y_end = 0.0f;

        void set_code_id(int id);
        int get_code_id() const;


    protected:
        int code_id = -1;

        mutable std::mutex mutex;


};

class Markers {
    public:
        static std::vector<std::shared_ptr<ArucoMarker>> registered_markers;
};