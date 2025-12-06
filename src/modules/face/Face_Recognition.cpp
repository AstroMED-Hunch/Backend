#include "Face_Recognition.hpp"

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/face.hpp>

std::string Face_Recognition::get_module_name() const {
    return "Face Recognition";
}

void Face_Recognition::initialize() {
    std::cout << "Face Recognition module initialized." << std::endl;
    const std::string face_recognition_model = layout->get_config_value("face_recognition_model");
    std::cout << "Using face recognition model: " << face_recognition_model << std::endl;

    const std::string face_detection_model = layout->get_config_value("face_detection_model");
    std::cout << "Using face recognition model: " << face_detection_model << std::endl;

    int input_width = 320;
    int input_height = 320;
    float score_threshold = 0.6f;
    float nms_threshold = 0.3f;
    int top_k = 5000;

    try {
        const std::string w = layout->get_config_value("face_detection_input_width");
        if (!w.empty()) input_width = std::stoi(w);
        const std::string h = layout->get_config_value("face_detection_input_height");
        if (!h.empty()) input_height = std::stoi(h);
        const std::string st = layout->get_config_value("face_detection_score_threshold");
        if (!st.empty()) score_threshold = std::stof(st);
        const std::string nt = layout->get_config_value("face_detection_nms_threshold");
        if (!nt.empty()) nms_threshold = std::stof(nt);
        const std::string tk = layout->get_config_value("face_detection_top_k");
        if (!tk.empty()) top_k = std::stoi(tk);
    } catch (const std::exception &e) {
        std::cerr << "Failed parsing face detection config, using defaults: " << e.what() << std::endl;
    }

    std::cout << "Face detector params: size=" << input_width << "x" << input_height
              << ", score_thresh=" << score_threshold << ", nms_thresh=" << nms_threshold
              << ", top_k=" << top_k << "\"" << std::endl;

    face_detector = cv::FaceDetectorYN::create(face_detection_model, "", cv::Size(input_width, input_height), score_threshold, nms_threshold, top_k);
    face_recognizer = cv::FaceRecognizerSF::create(face_recognition_model, "");
}

void Face_Recognition::run(cv::Mat cap) {
    cv::Mat faces;
    face_detector->setInputSize(cap.size());
    face_detector->detect(cap, faces);
    for (int i = 0; i < faces.rows; ++i) {
        cv::Mat face;
        face_recognizer->alignCrop(cap, faces, face);

        cv::Mat feature;
        face_recognizer->feature(face, feature);

        std::vector<cv::Point2f> landmarks;

        landmarks.push_back(cv::Point((int)faces.at<float>(i, 4), (int)faces.at<float>(i, 5)));
        landmarks.push_back(cv::Point((int)faces.at<float>(i, 6), (int)faces.at<float>(i, 7)));
        landmarks.push_back(cv::Point((int)faces.at<float>(i, 8), (int)faces.at<float>(i, 9)));
        landmarks.push_back(cv::Point((int)faces.at<float>(i, 10), (int)faces.at<float>(i, 11)));
        landmarks.push_back(cv::Point((int)faces.at<float>(i, 12), (int)faces.at<float>(i, 13)));

        for (const auto& point : landmarks) {
            cv::circle(cap, point, 2, cv::Scalar(0, 255, 0), -1);
        }



        std::cout << "Detected face " << i << " with feature vector of size " << feature.total() << std::endl;
    }
}

void Face_Recognition::shutdown() {
}
