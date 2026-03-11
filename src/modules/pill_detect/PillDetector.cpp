//
// Created by Marco Stulic on 3/10/26.
//

#include "PillDetector.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>

constexpr double PILL_DETECTOR_THRESHOLD = 0.8;
constexpr double PILL_NMS_THRESHOLD = 0.8;
const int INPUT_H = 480;
const int INPUT_W = 832;

PillDetector* PillDetector::instance = nullptr;

std::string PillDetector::get_module_name() const {
    return Module::get_module_name();
}

void PillDetector::initialize() {
    std::string model_path = layout->get_config_value("pill_detector_model");
    instance = this;
    if (model_path.empty()) {
        std::cerr << "PillDetector: No model path specified in layout config under 'pill_detector_model'" << std::endl;
        return;
    }

    try {
        net = cv::dnn::readNet(model_path);
        std::cout << "PillDetector: Successfully loaded model from " << model_path << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "PillDetector: Failed to load model from " << model_path << ": " << e.what() << std::endl;
    }

    orb = cv::ORB::create(1000);
    matcher = cv::BFMatcher(cv::NORM_HAMMING, true);

    // load images
    std::string dir = layout->get_config_value("pill_images_dir");
    if (dir.empty()) {
        std::cerr << "PillDetector: No pill images directory specified in layout config under 'pill_images_dir'" << std::endl;
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            const std::string path = entry.path().string();
            const std::string filename = entry.path().stem().string();
            cv::Mat img = cv::imread(path);
            if (img.empty()) {
                std::cerr << "PillDetector: Failed to load reference image from " << path << std::endl;
                continue;
            }
            pill_ref_images[filename] = img;
            std::cout << "PillDetector: Loaded reference image for " << filename << " from " << path << std::endl;
        }
    }

}

int PillDetector::get_match_score(const cv::Mat& descriptors_scene, const std::string& pill_name) {
    if (pill_ref_images.find(pill_name) == pill_ref_images.end()) {
        std::cerr << "PillDetector: Reference image not found for " << pill_name << std::endl;
        return 0;
    }

    cv::Mat& img_object = pill_ref_images[pill_name];
    if (img_object.empty()) {
        return 0;
    }

    std::vector<cv::KeyPoint> keypoints_object;
    cv::Mat descriptors_object;
    if (img_object.cols >= 32 && img_object.rows >= 32) {
        orb->detectAndCompute(img_object, cv::noArray(), keypoints_object, descriptors_object);
    }

    if (descriptors_object.empty() || descriptors_scene.empty()) {
        return 0;
    }

    std::vector<cv::DMatch> matches;
    matcher.match(descriptors_object, descriptors_scene, matches);

    return matches.size();
}

void PillDetector::run(cv::Mat cap) {
    if (!waiting_for_result) return;
    if (cap.empty()) {
        std::cerr << "PillDetector: Input frame is empty." << std::endl;
        return;
    }

    if (net.empty()) {
        std::cerr << "PillDetector: Model is not loaded." << std::endl;
        waiting_for_result = false;
        if (result_callback) result_callback({});
        return;
    }

    const cv::Mat frame = cap.clone();
    last_detected_pill_images.clear();

    cv::Mat blob = cv::dnn::blobFromImage(
        frame, 1.0 / 255.0, cv::Size(INPUT_W, INPUT_H), cv::Scalar(), true, false, CV_32F
    );

    net.setInput(blob);
    cv::Mat raw = net.forward();

    cv::Mat detections;
    int num_features;
    if (raw.dims == 3 && raw.size[1] < raw.size[2]) {
        num_features = raw.size[1];
        cv::Mat temp = raw.reshape(1, num_features);
        cv::transpose(temp, detections);
    } else {
        num_features = raw.size[raw.dims - 1];
        detections = raw.reshape(1, static_cast<int>(raw.total() / num_features));
    }

    std::vector<cv::Rect> raw_boxes;
    std::vector<float> confidences;

    for (int i = 0; i < detections.rows; ++i) {
        const float* det = detections.ptr<float>(i);
        const float conf = (num_features >= 6) ? det[4] * det[5] : det[4];

        if (conf < PILL_DETECTOR_THRESHOLD) continue;

        float cx = det[0];
        float cy = det[1];
        float w  = det[2];
        float h  = det[3];

        int width  = static_cast<int>(w / INPUT_W * frame.cols);
        int height = static_cast<int>(h / INPUT_H * frame.rows);
        int x      = static_cast<int>((cx / INPUT_W * frame.cols) - (width / 2));
        int y      = static_cast<int>((cy / INPUT_H * frame.rows) - (height / 2));

        cv::Rect unclamped_box(x, y, width, height);
        cv::Rect frame_bounds(0, 0, frame.cols, frame.rows);
        cv::Rect box = unclamped_box & frame_bounds;

        if (box.width > 0 && box.height > 0) {
            raw_boxes.push_back(box);
            confidences.push_back(conf);
        }
    }

    std::vector<int> nms_indices;
    cv::dnn::NMSBoxes(raw_boxes, confidences, PILL_DETECTOR_THRESHOLD, PILL_NMS_THRESHOLD, nms_indices);

    std::vector<std::pair<cv::Rect, float>> pill_boxes;
    for (int idx : nms_indices) {
        pill_boxes.emplace_back(raw_boxes[idx], confidences[idx]);
    }

    cv::Mat annotated = frame.clone();
    PillResult results;

    for (const auto& [box, conf] : pill_boxes) {
        last_detected_pill_images.push_back(frame(box).clone());

        cv::rectangle(annotated, box, cv::Scalar(0, 255, 0), 2);
        std::string label = "pill " + cv::format("%.2f", conf);
        cv::putText(annotated, label, cv::Point(box.x, box.y - 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);

        cv::Mat pill_roi = frame(box).clone();
        cv::Mat gray_roi;
        cv::cvtColor(pill_roi, gray_roi, cv::COLOR_BGR2GRAY);

        std::vector<cv::KeyPoint> kp_scene;
        cv::Mat descriptors_scene;

        if (gray_roi.cols >= 32 && gray_roi.rows >= 32) {
            orb->detectAndCompute(gray_roi, cv::noArray(), kp_scene, descriptors_scene);
        }

        std::string best_pill_name = "misc";
        int best_score = 0;

        if (!descriptors_scene.empty()) {
            for (const auto& [pill_name, ref_img] : pill_ref_images) {
                int score = get_match_score(descriptors_scene, pill_name);
                if (score > best_score) {
                    best_score = score;
                    best_pill_name = pill_name;
                }
            }
        }
        detected(results, best_pill_name);
    }

    cv::imshow("Pill Detector", annotated);
    cv::imwrite("pill_detector_output.jpg", annotated);

    waiting_for_result = false;
    if (result_callback) {
        result_callback(std::move(results));
    }
}

void PillDetector::shutdown() {
}

void PillDetector::detected(PillResult& pill_result, std::string name) {
    auto exists = std::ranges::find_if(pill_result, [&name](const auto& entry) -> bool {
        return (entry.pill_type == name);
    });

    if (exists == pill_result.end()) {
        pill_result.emplace_back(name, 1);
    } else {
        ++exists->quantity;
    }
}

bool PillDetector::is_waiting_for_result() const {
    return waiting_for_result;
}

void PillDetector::get_result(std::function<void(PillResult)> callback) {
    if (waiting_for_result) {
        std::cerr << "PillDetector: Already waiting for a result, cannot get new result until current one is ready." << std::endl;
        return;
    }

    waiting_for_result = true;
    result_callback = std::move(callback);
}

const std::vector<cv::Mat>& PillDetector::get_last_detected_pill_images() const {
    return last_detected_pill_images;
}

PillDetector* PillDetector::get() {
    return instance;
}