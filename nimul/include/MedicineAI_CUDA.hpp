#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace MedicineAI {
namespace CUDA {

/**
 * GPU Accelerator for computationally intensive operations made by Nimul Islam 
 * Optimized for NVIDIA Jetson Nano (Maxwell architecture, CUDA Compute 5.3) 
 * 
 * some features:
 * cuBLAS for optimized matrix operations
 * more speed for batch predictions and training
 */
class GPUAccelerator {
public:
    GPUAccelerator();
    ~GPUAccelerator();
    
    /**
     * Initialize GPU resources (cuBLAS, cuSOLVER)
     * Must be called before any GPU operations
     * @return true if successful
     */
    bool Initialize();
    
    /**
     * Check if CUDA is available on this system
     * @return true if CUDA device detected
     */
    static bool IsAvailable();
    
    /**
     * Extract features from adherence logs in parallel on GPU
     * Significantly faster than CPU for large datasets (>100 samples)
     * 
     * @param timestamps Vector of event timestamps
     * @param doses Vector of doses taken
     * @param is_returns Vector indicating if event was a return
     * @param current_time Current timestamp for relative calculations
     * @param features_out Output feature matrix (n_samples x n_features)
     * @return true if successful
     */
    bool ExtractFeatures(
        const std::vector<int64_t>& timestamps,
        const std::vector<double>& doses,
        const std::vector<bool>& is_returns,
        int64_t current_time,
        std::vector<std::vector<double>>& features_out
    );
    
    /**
     * Train ridge regression model on GPU using cuBLAS/cuSOLVER
     * Much faster than CPU dlib for datasets >50 samples
     * 
     * @param X Feature matrix (n_samples x n_features)
     * @param y Target vector (n_samples)
     * @param lambda Regularization parameter
     * @param weights_out Output model weights
     * @param feature_means_out Output feature means for normalization
     * @param feature_stds_out Output feature stds for normalization
     * @return true if successful
     */
    bool TrainRidgeRegression(
        const std::vector<std::vector<double>>& X,
        const std::vector<double>& y,
        double lambda,
        std::vector<double>& weights_out,
        std::vector<double>& feature_means_out,
        std::vector<double>& feature_stds_out
    );
    
    /**
     * Batch prediction on GPU
     * Ideal for predicting usage for multiple users simultaneously
     * 
     * @param X Feature matrix (n_samples x n_features)
     * @param weights Model weights
     * @param feature_means Feature means for normalization
     * @param feature_stds Feature stds for normalization
     * @param predictions_out Output predictions
     * @return true if successful
     */
    bool PredictBatch(
        const std::vector<std::vector<double>>& X,
        const std::vector<double>& weights,
        const std::vector<double>& feature_means,
        const std::vector<double>& feature_stds,
        std::vector<double>& predictions_out
    );
    
    /**
     * Compute statistics (mean, std, min, max) in parallel on GPU
     * Uses Thrust library for optimized reductions
     * 
     * @param data Input data vector
     * @param mean_out Output mean
     * @param std_dev_out Output standard deviation
     * @param min_out Output minimum value
     * @param max_out Output maximum value
     * @return true if successful
     */
    bool ComputeStatistics(
        const std::vector<double>& data,
        double& mean_out,
        double& std_dev_out,
        double& min_out,
        double& max_out
    );
    
    /**
     * Compute median using GPU sorting
     * Faster than CPU for large datasets
     * 
     * @param data Input data vector (will be modified)
     * @return Median value
     */
    double ComputeMedian(std::vector<double>& data);
    
    /**
     * Get GPU memory usage info
     * Useful for monitoring Jetson Nano's limited 4GB RAM
     * 
     * @param free_mb Output: free memory in MB
     * @param total_mb Output: total memory in MB
     * @return true if successful
     */
    bool GetMemoryInfo(size_t& free_mb, size_t& total_mb);
    
    /**
     * Synchronize all GPU operations
     * Ensures all kernels have completed
     */
    void Synchronize();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * Performance profiler for GPU operations
 * Helps optimize performance on Jetson Nano
 */
class GPUProfiler {
public:
    GPUProfiler(const std::string& name);
    ~GPUProfiler();
    
    /**
     * Get elapsed time in milliseconds
     */
    float GetElapsedTime() const;
    
private:
    std::string operation_name;
    cudaEvent_t start_event;
    cudaEvent_t stop_event;
};

/**
 * Unified memory helper for Jetson Nano
 * Simplifies memory management with automatic migration
 */
class UnifiedMemoryManager {
public:
    /**
     * Allocate unified memory (accessible from CPU and GPU)
     * @param size Size in bytes
     * @return Pointer to allocated memory
     */
    static void* Allocate(size_t size);
    
    /**
     * Free unified memory
     * @param ptr Pointer to free
     */
    static void Free(void* ptr);
    
    /**
     * Prefetch memory to GPU for better performance
     * @param ptr Pointer to memory
     * @param size Size in bytes
     */
    static void PrefetchToGPU(void* ptr, size_t size);
    
    /**
     * Prefetch memory to CPU
     * @param ptr Pointer to memory
     * @param size Size in bytes
     */
    static void PrefetchToCPU(void* ptr, size_t size);
};

/**
 * GPU-accelerated matrix operations
 */
class GPUMatrix {
public:
    GPUMatrix(int rows, int cols);
    ~GPUMatrix();
    
    /**
     * Load data from CPU vector
     */
    void LoadFromCPU(const std::vector<double>& data);
    
    /**
     * Copy data to CPU vector
     */
    void CopyToCPU(std::vector<double>& data) const;
    
    /**
     * Matrix multiplication: C = A * B
     */
    static bool Multiply(const GPUMatrix& A, const GPUMatrix& B, GPUMatrix& C);
    
    /**
     * Matrix transpose
     */
    GPUMatrix Transpose() const;
    
    /**
     * Add scalar to all elements
     */
    void AddScalar(double scalar);
    
    /**
     * Element-wise multiplication
     */
    void ElementWiseMultiply(const GPUMatrix& other);
    
    int GetRows() const { return rows; }
    int GetCols() const { return cols; }
    
private:
    int rows, cols;
    double* d_data;
};

} // namespace CUDA
} // namespace MedicineAI

// Convenience macros for CUDA error checking in user code
#define MEDICINE_AI_CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error: " << cudaGetErrorString(err) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while(0)

// Macro to profile GPU operations
#define MEDICINE_AI_PROFILE_GPU(name) \
    MedicineAI::CUDA::GPUProfiler profiler_##__LINE__(name);