//
// Created by Marco Stulic on 3/10/26.
//

#pragma once
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/core/mat.hpp>
#include <vector>
#include <opencv2/features2d.hpp>

#include "Module.hpp"
#include "Module_Registry.hpp"
#include "models/PillEntry.hpp"

class PillDetector : public Module {
public:
    using PillResult = std::vector<PillEntry>;
    [[nodiscard]] std::string get_module_name() const override;
    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;
    [[nodiscard]] bool is_waiting_for_result() const;
    void get_result(std::function<void(PillResult)> callback);
    [[nodiscard]] const std::vector<cv::Mat>& get_last_detected_pill_images() const;
    int get_match_score(const cv::Mat& descriptors_scene, const std::string& image_path);

    static void detected(PillResult& pill_result, std::string name);
protected:
    cv::dnn::Net net;
    bool waiting_for_result = false;
    std::function<void(PillResult)> result_callback;
    std::vector<cv::Mat> last_detected_pill_images;
    cv::Ptr<cv::ORB> orb;
    cv::BFMatcher matcher;
    std::unordered_map<std::string, cv::Mat> pill_ref_images;
};

MAKE_MODULE(PillDetector, "pill_detector");