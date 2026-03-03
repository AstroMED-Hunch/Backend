---

# Hunch Backend Setup

'astroMed's Hunch Medical setup is a modular computer vision and AI-powered system designed for intelligent shelf and inventory management in medical environments.'
It provides real-time shelf monitoring and analysis using optical recognition, ArUco markers, face detection, and medical AI inference. We introduce our specialized medicineAI framework for ML & smart mangement. 

This system integrates:

* Optical recognition (OpenCV)
* Face detection using YuNet (OpenCV DNN)
* Shelf layout parsing and recognition
* ArUco marker detection
* Medical AI inference (dlib)
* REST API services
* WebSocket kiosk communication

> CUDA files exist in the repository but are **not used** in this version. Ignore all CUDA-related files during setup.

---

## Project Structure

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

## System Requirements

* C++20 compiler
* CMake 3.15+
* OpenCV (image processing, ArUco detection, face detection)
* dlib (medical AI inference)
* IXWebSocket (WebSocket communications)
* nlohmann/json (REST API handling)

**Supported platforms:** Linux, macOS (Windows compatibility exists)
Ubuntu 22.04 is recommended.

---

## Installation Guide

### 1. Install OpenCV

OpenCV is used for:

* Optical recognition
* Shelf detection
* ArUco markers
* Face detection (YuNet ONNX model)

**Ubuntu/Debian:**

```bash
sudo apt update
sudo apt install libopencv-dev
```

Check installation:

```bash
pkg-config --modversion opencv4
```

If ArUco or DNN modules are missing, build OpenCV from source:

```bash
cmake -D BUILD_opencv_aruco=ON \
      -D BUILD_opencv_dnn=ON ..
```

**macOS (Homebrew):**

```bash
brew install opencv cmake
```

---

### 2. Install dlib (Required for MedicalAI)

**Ubuntu/Debian:**

```bash
sudo apt install build-essential cmake
sudo apt install libx11-dev libopenblas-dev liblapack-dev
git clone https://github.com/davisking/dlib.git
cd dlib
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

**macOS (Homebrew):**

```bash
brew install dlib
```

---

### 3. Build the Project

From the root of the repository:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Do **not** enable CUDA. Ignore:

* `MedicineAI_CUDA.cu`
* `MedicineAI_CUDA.hpp`

If CMake cannot find OpenCV:

```bash
cmake -D OpenCV_DIR=/usr/local/lib/cmake/opencv4 ..
```

The compiled executable will be located at:

```
build/HunchMedical
```

---

## 4. Run the System

**Default camera (index 0):**

```bash
./build/HunchMedical
```

**Custom camera device:**

```bash
./build/HunchMedical --camera 1
```

**Configuration file:**
By default, the application loads:

```
extern/shelf_testing.mlayout
```

You can modify the layout path in `src/main.cpp` to use a different file.

---

## Module System

Hunch Medical uses a modular architecture. Modules are loaded dynamically based on the `.mlayout` configuration.

To add a module:

1. Create a class inheriting from `Module`
2. Implement required methods: `get_module_name()`, `initialize()`, `run()`, `shutdown()`
3. Register the module using `MAKE_MODULE`
4. Request the module in the `.mlayout` file

Modules process video frames from a camera or video file.

---

## Configuration

Layout files (`.mlayout`) define:

* Shelf and box layouts with ArUco marker IDs
* Modules to load
* Person/face mappings
* Marker ID → display name mappings
* System configuration values (e.g., camera index)

---

## Module Overview

### Layout Parsing

* Lexer
* Parser
* `.mlayout` files
* ShelfDatabase

Builds the internal shelf model from configuration files.

### Shelf Recognition

* ShelfRecognition
* GeomUtils
* Shelf_Bounds

Uses OpenCV geometry and spatial calculations.

### ArUco Detection

* Aruco
* Code

Handles marker-based calibration and positioning.

### Face Recognition

* Face_Recognition
* `yunet.onnx`

Uses OpenCV DNN for detection and tracking.

### Medical AI

* MedicineAI
* Integration

Runs dlib-based inference logic. CUDA files are **not active**.

### REST API

* RestHandler

Handles HTTP requests and data exchange.

### Kiosk Communication

* KioskEventHandler
* IXWebSocket

Manages real-time communication between kiosk UI and backend.

---

## Common Issues

**OpenCV Not Found**

```bash
cmake -D OpenCV_DIR=/path/to/opencv/build ..
```

**dlib Linking Errors**

```bash
sudo ldconfig
```

Ensure `/usr/local/lib` is in the linker path.

**YuNet Model Not Loading**

* Check `yunet.onnx` is in the correct directory
* Confirm OpenCV DNN is enabled

---

## Deployment Notes

Before NASA HUNCH demos, test:

1. Load a `.mlayout` file
2. Detect shelves
3. Detect faces
4. Run MedicalAI inference
5. Test REST endpoints
6. Connect kiosk WebSocket

If each module runs individually, the full system should operate correctly.

---

## License

Developed for educational use under NASA HUNCH.
Third-party libraries retain their respective licenses.

---
