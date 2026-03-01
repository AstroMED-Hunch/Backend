#include "Face_Recognition.hpp"

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/face.hpp>
#include <opencv2/imgcodecs.hpp>

#include "modules/kiosk_comms/KioskEventHandler.hpp"

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

    for (const auto& person : layout->people) {
        const std::string& name = person.first;
        const std::string& image_path = person.second;
        cv::Mat img = cv::imread(image_path);
        if (img.empty()) {
            std::cerr << "Failed to load face image for " << name << " from " << image_path << std::endl;
            continue;
        }
        face_detector->setInputSize(img.size());
        cv::Mat faces;
        face_detector->detect(img, faces);
        if (faces.rows > 0) {
            cv::Mat aligned_img;
            face_recognizer->alignCrop(img, faces.row(0), aligned_img);
            cv::Mat feature;
            face_recognizer->feature(aligned_img, feature);
            known_face_embeddings[name] = feature.clone();
            std::cout << "Loaded face for " << name << " from " << image_path << std::endl;
        }
        else {
            std::cerr << "No face detected in image for " << name << " from " << image_path << std::endl;
        }
    }
}

void Face_Recognition::run(cv::Mat cap) {
    cv::Mat faces;
    face_detector->setInputSize(cap.size());
    face_detector->detect(cap, faces);
    if (faces.rows < 1) {
        return;
    }
    std::vector<std::string> detected_names;
    detected_names.reserve(faces.rows); // prevent realloc
    for (int i = 0; i < faces.rows; ++i) {

        cv::Mat face;
        face_recognizer->alignCrop(cap, faces.row(i), face);

        cv::Mat feature;
        face_recognizer->feature(face, feature);

        std::string best_name = "Unknown";
        double max_similarity = 0.0;

        double threshold = 0.45;

        for (const auto& known_face : known_face_embeddings) {
            const std::string& name = known_face.first;
            const cv::Mat& known_feature = known_face.second;

            double similarity = face_recognizer->match(feature, known_feature, cv::FaceRecognizerSF::FR_COSINE);

            // std::cout << name << ": " << similarity << std::endl;

            if (similarity > max_similarity) {
                max_similarity = similarity;
                if (similarity > threshold) {
                    best_name = name;
                }
            }
        }

        if (best_name != "Unknown") {
            float x = faces.at<float>(i, 0);
            float y = faces.at<float>(i, 1);
            float w = faces.at<float>(i, 2);
            float h = faces.at<float>(i, 3);

            int ix = std::max(0, std::min((int)std::round(x), cap.cols - 1));
            int iy = std::max(0, std::min((int)std::round(y), cap.rows - 1));
            int iw = std::max(0, std::min((int)std::round(w), cap.cols - ix));
            int ih = std::max(0, std::min((int)std::round(h), cap.rows - iy));

            cv::Rect box(ix, iy, iw, ih);
            cv::rectangle(cap, box, cv::Scalar(0, 255, 0), 2);
            std::string label = best_name + " (" + std::to_string(max_similarity).substr(0,4) + ")";
            cv::putText(cap, label, cv::Point(box.x, box.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 255, 0), 2);
            detected_names.push_back(best_name);
        }
    }
    detected_names.shrink_to_fit();

    if (detected_names.size() > 1) {
        KioskEventHandler::get()->send_face_recogniton_update("err_multiplicity");
    } else if (detected_names.size() == 1) {
        KioskEventHandler::get()->send_face_recogniton_update(detected_names[0]);
    } else {
        KioskEventHandler::get()->send_face_recogniton_update("err_nodetect");
    }
}

void Face_Recognition::shutdown() {
}
