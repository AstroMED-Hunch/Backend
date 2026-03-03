# Hunch Backend Setup 

Modular C++ computer vision and medical inference platform developed for the NASA HUNCH Medical Inventory System.

This system integrates:

* Optical recognition (OpenCV)
* Face detection using YuNet (OpenCV DNN)
* Shelf layout parsing and recognition
* ArUco marker detection
* Medical AI inference (dlib + CUDA)
* REST API services
* WebSocket kiosk communication

CUDA files exist in the repository but are not used in this version. Ignore all CUDA-related files during setup.

---

# Project Structure

```
extern/
  libs/IXWebSocket/

src/
  main.cpp / main.hpp
  Lexer.*
  Parser.*
  BoxEntry.*
  ShelfEntry.*
  ShelfDatabase.*
  Interpreters.*
  Module.*
  Module_Registry.*
  Vec.hpp

  modules/
    aruco/
    face/
    medical_ai/
    kiosk_comms/
    shelf_recognition/

  rest_handler/

models/
mlayout/

example.mlayout
shelf_testing.mlayout
yunet.onnx
api_functions.md
```

---

# System Requirements

* C++17 or newer
* CMake 3.16+
* GCC or Clang
* Linux or macOS (windows compatibility exists as well) 

Ubuntu 22.04 is recommended.

---

# Installation Guide

## 1. Install OpenCV

OpenCV is used for:

* Optical recognition
* Shelf detection
* ArUco markers
* Face detection (YuNet ONNX model)

On Ubuntu:

```bash
sudo apt update
sudo apt install libopencv-dev
```

Check installation:

```bash
pkg-config --modversion opencv4
```

If that prints a version number, OpenCV is installed.

If ArUco or DNN modules are missing, build OpenCV from source with:

```bash
cmake -D BUILD_opencv_aruco=ON \
      -D BUILD_opencv_dnn=ON ..
```

---

## 2. Install dlib (Required for MedicalAI)

MedicalAI depends on dlib.

Install required tools:

```bash
sudo apt install build-essential cmake
sudo apt install libx11-dev libopenblas-dev liblapack-dev
```

Clone and build:

```bash
git clone https://github.com/davisking/dlib.git
cd dlib
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

If linking errors appear later, run:

```bash
sudo ldconfig
```

---

## 3. Build the Project

From the root of the repository:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Do not enable CUDA. Ignore:

* MedicineAI_CUDA.cu
* MedicineAI_CUDA.hpp

If CMake cannot find OpenCV:

```bash
cmake -D OpenCV_DIR=/usr/local/lib/cmake/opencv4 ..
```

---

## 4. Run the System

From inside the `build` folder:

```bash
./NASA_HUNCH_System
```

Before running, verify these files are present in the working directory:

* `yunet.onnx`
* `example.mlayout` or `shelf_testing.mlayout`
* `models/` folder (if used by MedicalAI)

If YuNet fails to load, check:

* The file path is correct
* OpenCV was built with DNN support

---

# Module Overview

## Layout Parsing

* Lexer
* Parser
* `.mlayout` files
* ShelfDatabase

Builds the internal shelf model from configuration files.

## Shelf Recognition

* ShelfRecognition
* GeomUtils
* Shelf_Bounds

Uses OpenCV geometry and spatial calculations.

## ArUco Detection

* Aruco
* Code

Handles marker-based calibration and positioning.

## Face Recognition

* Face_Recognition
* yunet.onnx

Uses OpenCV DNN for face detection and processing.

## Medical AI

* MedicineAI
* Integration

Runs dlib-based inference logic. CUDA files are not active in this build.

## REST API

* RestHandler

Handles HTTP requests and external communication.

## Kiosk Communication

* KioskEventHandler
* IXWebSocket

Manages real-time communication between the kiosk interface and backend.

---

# Common Issues

### OpenCV Not Found

Specify the path manually:

```bash
cmake -D OpenCV_DIR=/path/to/opencv/build ..
```

---

### dlib Link Errors

Run:

```bash
sudo ldconfig
```

Make sure `/usr/local/lib` is in the linker path.

---

### YuNet Model Not Loading

Confirm:

* `yunet.onnx` is in the correct directory
* OpenCV was built with DNN enabled

---

# Notes for NASA HUNCH Deployment

test:

* Loading a `.mlayout` file
* Shelf detection
* Face detection
* MedicalAI inference
* REST endpoints
* WebSocket kiosk connection

If each module runs individually without errors, the full pipeline should operate correctly.

---
