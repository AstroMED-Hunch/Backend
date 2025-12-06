#pragma once
#include "Module.hpp"
#include "Module_Registry.hpp"
#include <opencv2/face.hpp>
#include <opencv2/dnn.hpp>

class Face_Recognition final : public Module {
public:
    ~Face_Recognition() override = default;
    [[nodiscard]] std::string get_module_name() const override;

    void initialize() override;
    void run(cv::Mat cap) override;
    void shutdown() override;
protected:
    cv::Ptr<cv::FaceDetectorYN> face_detector;
    cv::Ptr<cv::FaceRecognizerSF> face_recognizer;

};

MAKE_MODULE(Face_Recognition, "face_recognition");
