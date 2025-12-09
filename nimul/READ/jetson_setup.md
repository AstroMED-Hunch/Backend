# What we need to deploy on the Jettison:
```
JetPack SDK (includes CUDA, cuDNN),
OpenCV (with CUDA support),
dlib (built with CUDA enabled),
My complied MedicineAI Library
```
# Prerequisites
```bash
sudo apt-get update
sudo apt-get upgrade

# Install CUDA Toolkit 
sudo apt-get install cuda-toolkit-10-2

# Install cuDNN
sudo apt-get install libcudnn8 libcudnn8-dev

# Install OpenCV with CUDA support
sudo apt-get install libopencv-dev

# Install dlib (build from source for best performance)
git clone https://github.com/davisking/dlib.git
cd dlib
mkdir build && cd build
cmake .. -DDLIB_USE_CUDA=ON -DUSE_AVX_INSTRUCTIONS=OFF
make -j4
sudo make install
```

# left here for config purposes in the future 
```bash
# Configure with CUDA enabled
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_CUDA=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_CUDA_ARCHITECTURES=53

# Build (use all 4 cores)
make -j4

# Install
sudo make install
```

# Compilation Command (this bs probably doesn't work)

```bash
# Compile example with CUDA support
nvcc -o medicine_ai_cuda_example \
    example_cuda.cpp \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lmedicine_ai \
    -lcublas \
    -lcusolver \
    -ldlib \
    -lopencv_core \
    -std=c++17 \
    -O3 \
    --use_fast_math \
    -gencode arch=compute_53,code=sm_53
```

GPU is faster when:
- Dataset has >50 samples
- Batch processing multiple predictions
- Training models
- Computing statistics on large arrays

CPU is faster when:
- Dataset has <50 samples (overhead dominates)
- Single prediction (not batched)
- Very small operations


# Integration with Marco's System

The CUDA acceleration is completely transparent to Marco's code so we don't really have to change anything as it just works faster

```cpp
// In Marco's main.cpp or module system
#include "Integration.cpp"  // Already has MedicineAI integration that I built to bridge

// The MedicineAI_Module automatically uses GPU if available (or at least I hope this functions as I coded it to)
MedicineAI_Module medicine_module;
medicine_module.initialize();  // Initializes GPU if present

// All operations automatically use GPU when beneficial
medicine_module.LogMedicationPickup("crew_001", "MED001", 2.0, marker_id);
medicine_module.GenerateResupplyReport(180);  // GPU accelerated
```

# Testing that CUDA WORKS

```cpp
#include "MedicineAI_CUDA.hpp"

int main() {
    if (MedicineAI::CUDA::GPUAccelerator::IsAvailable()) {
        std::cout << " CUDA is available" << std::endl;
        
        MedicineAI::CUDA::GPUAccelerator gpu;
        if (gpu.Initialize()) {
            std::cout << " GPU initialized successfully" << std::endl;
            
            size_t free_mb, total_mb;
            gpu.GetMemoryInfo(free_mb, total_mb);
            std::cout << " GPU Memory: " << free_mb << "/" 
                      << total_mb << " MB" << std::endl;
            
            return 0;
        }
    }
    
    std::cerr << " CUDA not available" << std::endl;
    return 1;
}
```

## Usage Example

```cpp
#include "MedicineAI.hpp"
#include "MedicineAI_CUDA.hpp"

int main() {
    // Initialize system
    MedicineAI::MedicineTracker tracker;
    MedicineAI::CUDA::GPUAccelerator gpu;
    gpu.Initialize();
    
    // Set Jetson to max performance
    system("sudo nvpmodel -m 0");
    system("sudo jetson_clocks");
    
    // Load configuration
    tracker.LoadConfiguration([](const std::string& key) {
        // Load from Marco's layout system
        return layout->get_config_value(key);
    });
    
    // Collect data from ArUco markers
    // (Marco's system automatically calls LogPickup/LogReturn thanks to nimul's amazing bridge)
    
    // Train models (GPU accelerated)
    tracker.TrainModels(true);
    
    // Generate predictions (batch on GPU)
    std::vector<std::string> crew = {"crew1", "crew2", "crew3"};
    for (const auto& member : crew) {
        auto pred = tracker.PredictUsage(member, "MED001", time(nullptr));
        std::cout << member << ": " << pred.predicted_daily_usage << std::endl;
    }
    
    // Generate ISS resupply report
    auto report = tracker.GenerateResupplyReport(
        time(nullptr) + 180*86400,  // 180 days from now
        180                          // mission duration
    );
    
    std::cout << "Resupply: " << report.recommendations.size() 
              << " medications" << std::endl;
    
    // 8. Export for mission planners
    tracker.ExportToCSV("./resupply_report.csv");
    
    return 0;
}
```
# Debugging CUDA Issues


# Things we can do for Best Performance (I asked my NVIDIA supervisor)

1. Use 10W mode for AI workloads (5W for battery operation)
2. Enable jetson_clocks before intensive operations
3. Batch operations whenever possible (3-5x speedup)
4. Monitor temperature and throttle if >80°C
5. Use unified memory for simplified memory management
6. Profile our code with `MEDICINE_AI_PROFILE_GPU` macro
7. Pre-allocate buffers to avoid repeated allocations
8. Consider data locality - keep hot data on GPU
9. Use async operations where possible for better overlap


# Troubleshooting
```
Problem: Out of memory errors
Solution: Reduce batch sizes, use streaming, close other applications

Problem: Slow performance
Solution: Check power mode, enable jetson_clocks, verify CUDA is being used

Problem: Thermal throttling
Solution: Improve cooling, reduce batch sizes, take breaks between operations

Problem: CUDA not found
Solution: Install JetPack SDK, verify with `nvcc --version`
```

# Check CUDA installation:

```bash
nvcc --version
```


## Performance 

### 1. Power Mode Selection

For best AI performance we should use 10W mode:

```bash
# Check current mode
sudo nvpmodel -q

# Set to 10W mode
sudo nvpmodel -m 0

# Set to 5W mode (for battery operation)
sudo nvpmodel -m 1
```

### 2. Jetson Clocks

we can max out on clock speeds for intensive operations:

```bash
# Enable maximum performance
sudo jetson_clocks

# Disable (return to dynamic clocking)
sudo jetson_clocks --restore
```


# Google Sheets dataset based on dlib + cuda (jetson taken into account)

| Operation | CPU Time | GPU Time | **Speedup** |
|-----------|----------|----------|-------------|
| Feature Extraction (100 samples) | 45ms | 8ms | **5.6x faster** |
| Model Training (100x15 matrix) | 120ms | 25ms | **4.8x faster** |
| Batch Predictions (50 users) | 80ms | 15ms | **5.3x faster** |
| Statistics (1000 values) | 12ms | 3ms | **4.0x faster** |


# Automatic GPU Acceleration
```cpp
// The system automatically uses GPU when available
MedicineAI::MedicineTracker tracker;
tracker.TrainModels(true); // Automatically uses GPU if initialized

// Or explicitly control GPU usage
MedicineAI::CUDA::GPUAccelerator gpu;
if (gpu.Initialize()) {
    // GPU operations enabled
    gpu.TrainRidgeRegression(X, y, lambda, weights, means, stds);
}
```

# Unified Memory Support
```cpp
// Allocate memory accessible from both CPU and GPU
double* data = (double*)UnifiedMemoryManager::Allocate(size);

// Prefetch to GPU for faster access
UnifiedMemoryManager::PrefetchToGPU(data, size);

// Use in GPU kernels...

// Free when done
UnifiedMemoryManager::Free(data);
```

# Batch Processing
```cpp
// Process multiple users at once
std::vector<std::vector<double>> features_batch;
// ... collect features for multiple users ...

std::vector<double> predictions;
gpu.PredictBatch(features_batch, weights, means, stds, predictions);
// 5x faster than individual predictions!
```

### Performance ProfilingG
GPU profiler I made  to optimize our code with
```cpp
{
    MEDICINE_AI_PROFILE_GPU("Feature Extraction");
    gpu.ExtractFeatures(timestamps, doses, is_returns, current_time, features);
    // Automatically prints: "Feature Extraction took 8.34 ms"
}
```

### 5. **Memory Monitoring**
```cpp
size_t free_mb, total_mb;
gpu.GetMemoryInfo(free_mb, total_mb);
std::cout << "GPU Memory: " << free_mb << "/" << total_mb << " MB" << std::endl;

if (free_mb < 500) {
    std::cerr << "Warning: Low GPU memory!" << std::endl;
    // Switch to smaller batches or CPU processing
}
```


# time to be GPU JUICY MAXING:

```bash
# Set to 10W mode (vs 5W efficiency mode)
sudo nvpmodel -m 0

# Max out clocks
sudo jetson_clocks

# Now run your application
./medicine_ai_cuda_example
```

The system automatically chooses the best processor:

```cpp
// Internally, the system decides:
if (gpu_available && num_samples > 50) {
    // Use GPU - significant speedup for larger datasets
    gpu.TrainRidgeRegression(...);
} else {
    // Use CPU - faster for small datasets due to transfer overhead
    cpu_ridge_regression(...);
}
```

# Thermal Management

According to nvidia documentation the Jetson Nano can get hot under sustained load:

```cpp
#include <fstream>

double GetGPUTemperature() {
    std::ifstream temp("/sys/devices/virtual/thermal/thermal_zone1/temp");
    double t = 0;
    if (temp.is_open()) {
        temp >> t;
        t /= 1000.0; // Convert to Celsius
    }
    return t;
}

// Adaptive processing
if (GetGPUTemperature() > 80.0) {
    // Reduce load: smaller batches or switch to CPU
    std::cout << "High temp, throttling GPU usage" << std::endl;
}
```

# Install thermal monitoring tool
```
sudo apt-get install jetson-stats
sudo jtop
```


# more batch processing stuff

```cpp
MedicineAI::MedicineTracker tracker;
MedicineAI::CUDA::GPUAccelerator gpu;
gpu.Initialize();

// Prepare batch of feature vectors
std::vector<std::vector<double>> batch_features;
std::vector<std::string> user_ids = {"user1", "user2", "user3", "user4"};

for (const auto& uid : user_ids) {
    auto features = /* extract features for user */;
    batch_features.push_back(features);
}

// Batch predict on GPU (much faster than individual predictions)
std::vector<double> predictions;
gpu.PredictBatch(batch_features, weights, means, stds, predictions);
```

# Might help with model Training Optimization

```cpp
// Train on GPU for datasets > 50 samples
if (num_samples > 50 && gpu.IsAvailable()) {
    // GPU training is 3-5x faster on Jetson Nano
    gpu.TrainRidgeRegression(X, y, lambda, weights, means, stds);
} else {
    // Fall back to CPU for small datasets
    // (CPU might be faster for <50 samples due to transfer overhead)
    tracker.TrainModels(true);
}
```
