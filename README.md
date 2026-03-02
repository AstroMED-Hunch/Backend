# Hunch Medical

Hunch Medical is a modular computer vision and AI-powered system designed for intelligent shelf and inventory management in medical environments. It leverages ArUco marker detection, face recognition, and machine learning to provide real-time monitoring and analysis of medical shelves and products.

## Project Overview

The system is built as a modular architecture where different processing modules can be independently loaded and executed. Key features include:

- **ArUco Marker Detection**: Identifies and tracks ArUco codes on shelves and products
- **Shelf Recognition**: Recognizes and monitors shelf layouts and product positions
- **Face Recognition**: Detects and identifies individuals interacting with shelves
- **Medical AI Integration**: Analyzes and processes medical-related data and configurations
- **REST API Handler**: Provides HTTP endpoints for external system integration
- **Kiosk Communications**: Handles real-time communication with kiosk devices via WebSockets
- **Audit Logging**: Maintains detailed logs of all system activities and interactions

## Prerequisites

Before building and running the project, ensure you have the following installed:

- **C++ Compiler**: Supporting C++20 standard
- **CMake**: Version 3.15 or higher
- **OpenCV**: Computer vision library (for image processing and ArUco detection)
- **DLib**: Machine learning library (for face recognition)
- **IXWebSocket**: WebSocket library for real-time communications

### Installing Dependencies

#### macOS (using Homebrew)
```bash
brew install opencv dlib cmake
```

#### Ubuntu/Debian
```bash
sudo apt-get install libopencv-dev libdlib-dev cmake
```
## Building the Project

1. **Clone/Navigate to the project directory**:
   ```bash
   cd /path/to/Hunch_Medical
   ```

2. **Create a build directory** (if not already present):
   ```bash
   mkdir -p build
   cd build
   ```

3. **Generate build files using CMake**:
   ```bash
   cmake ..
   ```

4. **Build the project**:
   ```bash
   cmake --build .
   ```

   Or using make directly:
   ```bash
   make
   ```

The compiled executable will be located at `build/HunchMedical`.

## Running the Application

### Basic Usage

Run the application with the default camera (camera index 0):
```bash
./build/HunchMedical
```

### Custom Camera Selection

Specify a different camera device:
```bash
./build/HunchMedical --camera 1
```

### Configuration

The application requires a layout configuration file in the MLayout format. By default, it loads from:
```
extern/shelf_testing.mlayout
```

You can modify the layout file path in `src/main.cpp` to use a different configuration.
```

## Module System

Hunch Medical uses a module-based architecture. Modules are dynamically loaded based on the layout configuration file and can be extended by:

1. Creating a new class inheriting from `Module`
2. Implementing required virtual methods: `get_module_name()`, `initialize()`, `run()`, and `shutdown()`
3. Registering the module using the `MAKE_MODULE` macro
4. Requesting the module in the MLayout configuration file

## Configuration

Layout configurations are defined in `.mlayout` files using a custom markup language. These files specify:
- Shelf and box layouts with ArUco marker assignments
- Module load requests
- Person/face mappings
- Marker ID to display name mappings
- System configuration values (e.g., camera index)

## Key Dependencies

- **OpenCV**: Image processing, ArUco detection, face detection and recognition
- **DLib**: Machine learning and face embedding generation
- **IXWebSocket**: Real-time WebSocket communications
- **nlohmann/json**: JSON serialization/deserialization for REST APIs

## Troubleshooting

### Build Issues
- Ensure all dependencies are properly installed
- Check that CMake can find the libraries: `cmake .. -DCMAKE_PREFIX_PATH=/path/to/dependencies`
- For macOS with M1/M2 chips, ensure you're using ARM-compatible builds

### Runtime Issues
- Verify the camera device index is correct (use `--camera` flag)
- Ensure the layout configuration file exists and is properly formatted
- Check that all required modules can be loaded without errors
- Monitor console output for detailed error messages

## Development Notes

- The project uses C++20 standard features
- Compiler warnings are treated as errors (controlled by CMake)
- The modular architecture allows for independent testing and development of components
- All modules operate on video frames from a capture device (webcam or video file)

