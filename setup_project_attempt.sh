#!/bin/bash

echo "hunch automation"
echo ""

# Get project root directory
PROJECT_ROOT=$(pwd)

echo "Project root: $PROJECT_ROOT"
echo ""

# Step 1: Verify directory structure
echo "Verifying directory structure..."

check_dir() {
    if [ -d "$1" ]; then
        echo "✓ $1"
        return 0
    else
        echo "✗ $1 (missing)"
        return 1
    fi
}

check_file() {
    if [ -f "$1" ]; then
        echo "✓ $1"
        return 0
    else
        echo "✗ $1 (missing)"
        return 1
    fi
}

ERRORS=0

# Check Marco's structure
check_dir "marco" || ((ERRORS++))
check_dir "marco/mlayout" || ((ERRORS++))
check_dir "marco/modules" || ((ERRORS++))
check_dir "marco/modules/aruco" || ((ERRORS++))
check_dir "marco/modules/face" || ((ERRORS++))

check_file "marco/main.cpp" || ((ERRORS++))
check_file "marco/Module.cpp" || ((ERRORS++))
check_file "marco/Module_Registry.cpp" || ((ERRORS++))
check_file "marco/mlayout/Lexer.cpp" || ((ERRORS++))
check_file "marco/mlayout/Parser.cpp" || ((ERRORS++))

# Check Nimul (MedicineAI) structure
check_dir "nimul" || ((ERRORS++))
check_dir "nimul/include" || ((ERRORS++))
check_dir "nimul/src" || ((ERRORS++))

check_file "nimul/include/MedicineAI.hpp" || ((ERRORS++))
check_file "nimul/include/MedicineAI_CUDA.hpp" || ((ERRORS++))
check_file "nimul/src/MedicineAI.cpp" || ((ERRORS++))
check_file "nimul/src/MedicineAI_C_API.cpp" || ((ERRORS++))
check_file "nimul/src/MedicineAI_CUDA.cu" || ((ERRORS++))
check_file "nimul/src/Integration.cpp" || ((ERRORS++))

echo ""
if [ $ERRORS -gt 0 ]; then
    echo " Warning: $ERRORS files/directories missing"
    echo "Please ensure all files are in place before building"
    echo ""
else
    echo " All files present"
    echo ""
fi

# Step 2: Create necessary directories
echo "Step 2: Creating data directories..."
mkdir -p medicine_data
mkdir -p nimul/examples
mkdir -p build
echo " Directories created"
echo ""

# Check dependencies
echo " Checking dependencies..."

check_command() {
    if command -v $1 &> /dev/null; then
        VERSION=$($1 --version 2>&1 | head -n 1)
        echo "✓ $1: $VERSION"
        return 0
    else
        echo "✗ $1: Not found"
        return 1
    fi
}

DEP_ERRORS=0

check_command cmake || ((DEP_ERRORS++))
check_command g++ || ((DEP_ERRORS++))

# Check for CUDA
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep "release" | sed -n 's/.*release \([0-9.]*\).*/\1/p')
    echo " CUDA: $CUDA_VERSION"
else
    echo " CUDA: Not found (will build without GPU acceleration)"
fi

# Check for OpenCV
if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    echo " OpenCV: $OPENCV_VERSION"
elif pkg-config --exists opencv; then
    OPENCV_VERSION=$(pkg-config --modversion opencv)
    echo " OpenCV: $OPENCV_VERSION"
else
    echo " OpenCV: Not found"
    ((DEP_ERRORS++))
fi

# Check for dlib
if [ -f "/usr/local/include/dlib/version.h" ] || [ -f "/usr/include/dlib/version.h" ]; then
    echo "dlib: Found"
else
    echo "dlib: Not found in standard locations"
fi

echo ""
if [ $DEP_ERRORS -gt 0 ]; then
    echo " Warning: Some dependencies are missing"
    echo "Install missing dependencies before building:"
    echo "  sudo apt-get install cmake g++ libopencv-dev"
    echo "  For CUDA: Install JetPack SDK"
    echo "  For dlib: Build from source with CUDA support"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Step 4: Configure CMake
echo "Step 4: Configuring CMake..."

cd build

# Detect if we should enable CUDA
if command -v nvcc &> /dev/null; then
    CUDA_OPTION="-DENABLE_CUDA=ON"
    echo "Configuring with CUDA support..."
else
    CUDA_OPTION="-DENABLE_CUDA=OFF"
    echo "Configuring without CUDA support..."
fi

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    $CUDA_OPTION \
    -DBUILD_EXAMPLES=ON

if [ $? -eq 0 ]; then
    echo "CMake configuration successful"
    echo ""
else
    echo " CMake configuration failed"
    echo ""
    echo "Common issues:"
    echo "  1. Missing CMakeLists.txt in project root"
    echo "  2. Missing dependencies (OpenCV, dlib)"
    echo "  3. Incorrect file paths in CMakeLists.txt"
    echo ""
    exit 1
fi

# Build
echo " Building project..."

# Detect number of cores
CORES=$(nproc)
echo "Building with $CORES parallel jobs..."

make -j$CORES

if [ $? -eq 0 ]; then
    echo ""
    echo " Build successful!"
    echo ""
else
    echo ""
    echo " Build failed"
    echo ""
    echo "Check the error messages above for details"
    exit 1
fi

# Verify build outputs
echo " Verifying build outputs..."

BUILD_ERRORS=0

check_file "iss_medical_tracker" || ((BUILD_ERRORS++))
check_file "libmarco_base.a" || ((BUILD_ERRORS++))
check_file "libaruco_module.a" || ((BUILD_ERRORS++))
check_file "libface_module.a" || ((BUILD_ERRORS++))
check_file "libmedicine_ai.a" || ((BUILD_ERRORS++))
check_file "libmedicine_ai_module.a" || ((BUILD_ERRORS++))

echo ""
if [ $BUILD_ERRORS -eq 0 ]; then
    echo " All build outputs present"
else
    echo " Warning: Some build outputs missing"
fi

cd ..

# Create run script
echo ""
echo " Creating run script..."

cat > run_tracker.sh << 'EOF'
#!/bin/bash

echo "Starting ISS Medical Tracker..."
echo ""

# Create data directory if it doesn't exist
mkdir -p medicine_data

# Check if running on Jetson Nano
if [ -f "/etc/nv_tegra_release" ]; then
    echo "Detected Jetson Nano - Setting performance mode..."
    
    # Set to max performance (requires sudo)
    sudo nvpmodel -m 0 2>/dev/null
    sudo jetson_clocks 2>/dev/null
    
    echo "Performance mode enabled"
    echo ""
fi

# Run the tracker
cd build
./iss_medical_tracker "$@"
EOF

chmod +x run_tracker.sh
echo " Created run_tracker.sh"

# lol
echo ""
echo "Setup Complete!"
echo ""
echo "Build outputs:"
echo "  Executable: build/iss_medical_tracker"
echo "  Libraries: build/*.a"
echo ""
echo "To run the tracker:"
echo "  ./run_tracker.sh --camera 0"
echo ""
echo "To rebuild:"
echo "  cd build && make -j$CORES"
echo ""
echo "To clean and rebuild:"
echo "  rm -rf build && ./$(basename $0)"
echo ""
echo "Data will be stored in: medicine_data/"
echo ""
