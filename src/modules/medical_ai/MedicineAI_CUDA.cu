// nimul's gpu accel for jetson nano made compatible w 11.x and 12.x
#include "MedicineAI_CUDA.hpp"
#include <cuda_runtime.h>
#include <cublas_v2.h>

// CUDA 12.x compatibility - cusolverDn API changed
#if CUDA_VERSION >= 12000
    #include <cusolverDn.h>
    #define CUSOLVER_LEGACY_API 0
#elif CUDA_VERSION >= 11000
    #include <cusolverDn.h>
    #define CUSOLVER_LEGACY_API 1
#else
    #include <cusolverDn.h>
    #define CUSOLVER_LEGACY_API 1
#endif

#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <thrust/reduce.h>
#include <iostream>
#include <vector>

namespace MedicineAI {
namespace CUDA {


#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " code=" << err << " \"" << cudaGetErrorString(err) << "\"" << std::endl; \
            return false; \
        } \
    } while(0)

#define CUBLAS_CHECK(call) \
    do { \
        cublasStatus_t status = call; \
        if (status != CUBLAS_SUCCESS) { \
            std::cerr << "cuBLAS error at " << __FILE__ << ":" << __LINE__ \
                      << " code=" << status << std::endl; \
            return false; \
        } \
    } while(0)


/**
 * Extract features from adherence logs in parallel
 * I optimized this for both Maxwell (5.3) and Ampere (8.7) architectures
 */
__global__ void ExtractFeaturesKernel(
    const double* timestamps,
    const double* doses,
    const bool* is_returns,
    int n_samples,
    int64_t current_time,
    double* features,
    int n_features
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Grid-stride loop for better GPU utilization
    for (int sample_idx = idx; sample_idx < n_samples; sample_idx += blockDim.x * gridDim.x) {
        if (sample_idx >= n_samples) return;

        // Feature 0: Sample index (position in sequence)
        features[sample_idx * n_features + 0] = (double)sample_idx;

        // Feature 1: Dose taken
        features[sample_idx * n_features + 1] = is_returns[sample_idx] ? 0.0 : doses[sample_idx];

        // Feature 2: Time since start (in hours)
        if (n_samples > 0) {
            double first_time = timestamps[0];
            features[sample_idx * n_features + 2] = (timestamps[sample_idx] - first_time) / 3600.0;
        }

        // Feature 3: Time until current (in hours)
        features[sample_idx * n_features + 3] = ((double)current_time - timestamps[sample_idx]) / 3600.0;

        // Feature 4: Time delta from previous (if exists)
        if (sample_idx > 0) {
            features[sample_idx * n_features + 4] = (timestamps[sample_idx] - timestamps[sample_idx-1]) / 3600.0;
        } else {
            features[sample_idx * n_features + 4] = 0.0;
        }

        // Feature 5: Cumulative dose so far
        double cumulative = 0.0;
        for (int i = 0; i <= sample_idx; ++i) {
            if (!is_returns[i]) cumulative += doses[i];
        }
        features[sample_idx * n_features + 5] = cumulative;

        // Feature 6-7: Hour of day (cyclical encoding)
        time_t t = (time_t)timestamps[sample_idx];
        struct tm* tm_info = localtime(&t);
        double hour = (double)tm_info->tm_hour;
        features[sample_idx * n_features + 6] = sin(2.0 * M_PI * hour / 24.0);
        features[sample_idx * n_features + 7] = cos(2.0 * M_PI * hour / 24.0);

        // Feature 8-9: Day of week (cyclical encoding)
        double day = (double)tm_info->tm_wday;
        features[sample_idx * n_features + 8] = sin(2.0 * M_PI * day / 7.0);
        features[sample_idx * n_features + 9] = cos(2.0 * M_PI * day / 7.0);

        // Feature 10: Is return flag
        features[sample_idx * n_features + 10] = is_returns[sample_idx] ? 1.0 : 0.0;

        // Feature 11: Local average (window of 5)
        double local_sum = 0.0;
        int count = 0;
        for (int i = max(0, sample_idx-2); i <= min(n_samples-1, sample_idx+2); ++i) {
            if (!is_returns[i]) {
                local_sum += doses[i];
                count++;
            }
        }
        features[sample_idx * n_features + 11] = count > 0 ? local_sum / count : 0.0;

        // Feature 12: Recent trend (last 3 samples slope)
        if (sample_idx >= 2) {
            double slope = (doses[sample_idx] - doses[sample_idx-2]) / 2.0;
            features[sample_idx * n_features + 12] = slope;
        } else {
            features[sample_idx * n_features + 12] = 0.0;
        }

        // Feature 13: Days since start
        if (n_samples > 0) {
            features[sample_idx * n_features + 13] = (timestamps[sample_idx] - timestamps[0]) / 86400.0;
        }

        // Feature 14: Usage frequency (doses per day)
        if (sample_idx > 0 && timestamps[sample_idx] > timestamps[0]) {
            double days_elapsed = (timestamps[sample_idx] - timestamps[0]) / 86400.0;
            features[sample_idx * n_features + 14] = days_elapsed > 0 ? (sample_idx + 1) / days_elapsed : 0.0;
        } else {
            features[sample_idx * n_features + 14] = 0.0;
        }
    }
}

/**
 * Normalize features: (x - mean) / std
 */
__global__ void NormalizeFeaturesKernel(
    double* features,
    const double* means,
    const double* stds,
    int n_samples,
    int n_features
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_samples * n_features) return;

    int feature_idx = idx % n_features;
    double std_val = stds[feature_idx];
    if (std_val < 1e-10) std_val = 1.0; // Avoid division by zero

    features[idx] = (features[idx] - means[feature_idx]) / std_val;
}

/**
 * Compute means for each feature column
 */
__global__ void ComputeMeansKernel(
    const double* features,
    double* means,
    int n_samples,
    int n_features
) {
    int feature_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (feature_idx >= n_features) return;

    double sum = 0.0;
    for (int i = 0; i < n_samples; ++i) {
        sum += features[i * n_features + feature_idx];
    }
    means[feature_idx] = sum / n_samples;
}

/**
 * Compute standard deviations for each feature column
 */
__global__ void ComputeStdDevsKernel(
    const double* features,
    const double* means,
    double* stds,
    int n_samples,
    int n_features
) {
    int feature_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (feature_idx >= n_features) return;

    double sum_sq_diff = 0.0;
    double mean = means[feature_idx];

    for (int i = 0; i < n_samples; ++i) {
        double diff = features[i * n_features + feature_idx] - mean;
        sum_sq_diff += diff * diff;
    }

    stds[feature_idx] = sqrt(sum_sq_diff / n_samples);
}

/**
 * Matrix-vector multiplication for prediction: y = X * w
 */
__global__ void MatVecMultKernel(
    const double* X,
    const double* weights,
    double* y,
    int n_samples,
    int n_features
) {
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= n_samples) return;

    double sum = 0.0;
    for (int col = 0; col < n_features; ++col) {
        sum += X[row * n_features + col] * weights[col];
    }
    y[row] = sum;
}

/**
 * Compute statistics in parallel using reduction
 */
__global__ void ComputeStatisticsKernel(
    const double* data,
    int n,
    double* sum,
    double* sum_sq,
    double* min_val,
    double* max_val
) {
    __shared__ double shared_sum[256];
    __shared__ double shared_sum_sq[256];
    __shared__ double shared_min[256];
    __shared__ double shared_max[256];

    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Initialize shared memory
    shared_sum[tid] = 0.0;
    shared_sum_sq[tid] = 0.0;
    shared_min[tid] = idx < n ? data[idx] : 1e10;
    shared_max[tid] = idx < n ? data[idx] : -1e10;

    // Load data
    if (idx < n) {
        double val = data[idx];
        shared_sum[tid] = val;
        shared_sum_sq[tid] = val * val;
        shared_min[tid] = val;
        shared_max[tid] = val;
    }

    __syncthreads();

    // Reduction
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (tid < stride) {
            shared_sum[tid] += shared_sum[tid + stride];
            shared_sum_sq[tid] += shared_sum_sq[tid + stride];
            shared_min[tid] = min(shared_min[tid], shared_min[tid + stride]);
            shared_max[tid] = max(shared_max[tid], shared_max[tid + stride]);
        }
        __syncthreads();
    }

    // Write result
    if (tid == 0) {
        atomicAdd(sum, shared_sum[0]);
        atomicAdd(sum_sq, shared_sum_sq[0]);
        atomicMin((int*)min_val, __double_as_longlong(shared_min[0]));
        atomicMax((int*)max_val, __double_as_longlong(shared_max[0]));
    }
}

// GPU ACCELERATOR CLASS IMPLEMENTATION

class GPUAccelerator::Impl {
public:
    cublasHandle_t cublas_handle;
    cusolverDnHandle_t cusolver_handle;
    bool is_initialized;

    // Device memory pools for reuse
    double* d_temp_features;
    double* d_temp_weights;
    double* d_temp_output;
    size_t temp_buffer_size;

    Impl() : is_initialized(false), d_temp_features(nullptr),
             d_temp_weights(nullptr), d_temp_output(nullptr),
             temp_buffer_size(0) {}

    ~Impl() {
        if (is_initialized) {
            Cleanup();
        }
    }

    bool Initialize() {
        if (is_initialized) return true;

        // Initialize cuBLAS
        cublasStatus_t cublas_status = cublasCreate(&cublas_handle);
        if (cublas_status != CUBLAS_SUCCESS) {
            std::cerr << "Failed to initialize cuBLAS" << std::endl;
            return false;
        }

        // Initialize cuSOLVER
        cusolverStatus_t cusolver_status = cusolverDnCreate(&cusolver_handle);
        if (cusolver_status != CUSOLVER_STATUS_SUCCESS) {
            std::cerr << "Failed to initialize cuSOLVER" << std::endl;
            cublasDestroy(cublas_handle);
            return false;
        }

        // Allocate temporary buffers (will grow as needed)
        temp_buffer_size = 1000 * 15 * sizeof(double); // 1000 samples, 15 features
        cudaMalloc(&d_temp_features, temp_buffer_size);
        cudaMalloc(&d_temp_weights, 15 * sizeof(double));
        cudaMalloc(&d_temp_output, 1000 * sizeof(double));

        is_initialized = true;

        // Print device info with version information
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        std::cout << "[MedicineAI CUDA] GPU Accelerator initialized" << std::endl;
        std::cout << "[MedicineAI CUDA] Device: " << prop.name << std::endl;
        std::cout << "[MedicineAI CUDA] Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "[MedicineAI CUDA] Total Global Memory: " << prop.totalGlobalMem / (1024*1024) << " MB" << std::endl;

        // Print CUDA version
        #if defined(CUDA_VERSION)
        int cuda_major = CUDA_VERSION / 1000;
        int cuda_minor = (CUDA_VERSION % 1000) / 10;
        std::cout << "[MedicineAI CUDA] CUDA Runtime Version: " << cuda_major << "." << cuda_minor << std::endl;

        #if CUDA_VERSION >= 12000
        std::cout << "[MedicineAI CUDA] Using CUDA 12.x API (modern)" << std::endl;
        #elif CUDA_VERSION >= 11000
        std::cout << "[MedicineAI CUDA] Using CUDA 11.x API (compatible)" << std::endl;
        #else
        std::cout << "[MedicineAI CUDA] Using CUDA 10.x API (legacy)" << std::endl;
        #endif
        #endif

        return true;
    }

    void Cleanup() {
        if (d_temp_features) cudaFree(d_temp_features);
        if (d_temp_weights) cudaFree(d_temp_weights);
        if (d_temp_output) cudaFree(d_temp_output);

        if (is_initialized) {
            cublasDestroy(cublas_handle);
            cusolverDnDestroy(cusolver_handle);
        }

        is_initialized = false;
    }
};


// here's the public api impl

GPUAccelerator::GPUAccelerator() : pImpl(new Impl()) {}

GPUAccelerator::~GPUAccelerator() = default;

bool GPUAccelerator::Initialize() {
    return pImpl->Initialize();
}

bool GPUAccelerator::IsAvailable() {
    int device_count = 0;
    cudaError_t error = cudaGetDeviceCount(&device_count);
    return (error == cudaSuccess && device_count > 0);
}

bool GPUAccelerator::ExtractFeatures(
    const std::vector<int64_t>& timestamps,
    const std::vector<double>& doses,
    const std::vector<bool>& is_returns,
    int64_t current_time,
    std::vector<std::vector<double>>& features_out
) {
    if (!pImpl->is_initialized) return false;

    int n_samples = timestamps.size();
    int n_features = 15;

    if (n_samples == 0) return true;

    // allocate device memory
    double *d_timestamps, *d_doses, *d_features;
    bool *d_is_returns;

    CUDA_CHECK(cudaMalloc(&d_timestamps, n_samples * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_doses, n_samples * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_is_returns, n_samples * sizeof(bool)));
    CUDA_CHECK(cudaMalloc(&d_features, n_samples * n_features * sizeof(double)));

    // convert timestamps to double and copy to device
    std::vector<double> timestamps_double(n_samples);
    for (int i = 0; i < n_samples; ++i) {
        timestamps_double[i] = (double)timestamps[i];
    }

    CUDA_CHECK(cudaMemcpy(d_timestamps, timestamps_double.data(),
                         n_samples * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_doses, doses.data(),
                         n_samples * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_is_returns, is_returns.data(),
                         n_samples * sizeof(bool), cudaMemcpyHostToDevice));

    // Launch kernel
    int block_size = 256;
    int grid_size = (n_samples + block_size - 1) / block_size;

    ExtractFeaturesKernel<<<grid_size, block_size>>>(
        d_timestamps, d_doses, d_is_returns, n_samples,
        current_time, d_features, n_features
    );

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Copy results back
    std::vector<double> features_flat(n_samples * n_features);
    CUDA_CHECK(cudaMemcpy(features_flat.data(), d_features,
                         n_samples * n_features * sizeof(double),
                         cudaMemcpyDeviceToHost));

    // Convert to 2D vector
    features_out.resize(n_samples);
    for (int i = 0; i < n_samples; ++i) {
        features_out[i].resize(n_features);
        for (int j = 0; j < n_features; ++j) {
            features_out[i][j] = features_flat[i * n_features + j];
        }
    }

    // Cleanup
    cudaFree(d_timestamps);
    cudaFree(d_doses);
    cudaFree(d_is_returns);
    cudaFree(d_features);

    return true;
}

bool GPUAccelerator::TrainRidgeRegression(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& y,
    double lambda,
    std::vector<double>& weights_out,
    std::vector<double>& feature_means_out,
    std::vector<double>& feature_stds_out
) {
    if (!pImpl->is_initialized) return false;
    if (X.empty() || y.empty()) return false;

    int n_samples = X.size();
    int n_features = X[0].size();

    // Flatten X matrix
    std::vector<double> X_flat(n_samples * n_features);
    for (int i = 0; i < n_samples; ++i) {
        for (int j = 0; j < n_features; ++j) {
            X_flat[i * n_features + j] = X[i][j];
        }
    }

    // allocate device memory
    double *d_X, *d_y, *d_means, *d_stds, *d_weights;

    CUDA_CHECK(cudaMalloc(&d_X, n_samples * n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_y, n_samples * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_means, n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_stds, n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_weights, n_features * sizeof(double)));

    // Copy data to device
    CUDA_CHECK(cudaMemcpy(d_X, X_flat.data(), n_samples * n_features * sizeof(double),
                         cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_y, y.data(), n_samples * sizeof(double),
                         cudaMemcpyHostToDevice));

    // Compute means
    int block_size = 256;
    int grid_size = (n_features + block_size - 1) / block_size;
    ComputeMeansKernel<<<grid_size, block_size>>>(d_X, d_means, n_samples, n_features);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Compute standard deviations
    ComputeStdDevsKernel<<<grid_size, block_size>>>(d_X, d_means, d_stds, n_samples, n_features);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Normalize features
    grid_size = (n_samples * n_features + block_size - 1) / block_size;
    NormalizeFeaturesKernel<<<grid_size, block_size>>>(d_X, d_means, d_stds, n_samples, n_features);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Solve ridge regression using cuBLAS: weights = (X^T * X + lambda * I)^-1 * X^T * y
    // For simplicity on Jetson Nano, we use normal equations
    // we can use cuSOLVER's QR or SVD for better numerical stability

    double *d_XtX, *d_Xty, *d_regularized;
    CUDA_CHECK(cudaMalloc(&d_XtX, n_features * n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_Xty, n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_regularized, n_features * n_features * sizeof(double)));

    // Compute X^T * X using cuBLAS
    double alpha = 1.0, beta = 0.0;
    CUBLAS_CHECK(cublasDgemm(pImpl->cublas_handle,
                            CUBLAS_OP_T, CUBLAS_OP_N,
                            n_features, n_features, n_samples,
                            &alpha, d_X, n_samples,
                            d_X, n_samples,
                            &beta, d_XtX, n_features));

    // Compute X^T * y using cuBLAS
    CUBLAS_CHECK(cublasDgemv(pImpl->cublas_handle,
                            CUBLAS_OP_T,
                            n_samples, n_features,
                            &alpha, d_X, n_samples,
                            d_y, 1,
                            &beta, d_Xty, 1));

    // Add regularization: X^T*X + lambda*I
    std::vector<double> XtX_host(n_features * n_features);
    CUDA_CHECK(cudaMemcpy(XtX_host.data(), d_XtX, n_features * n_features * sizeof(double),
                         cudaMemcpyDeviceToHost));

    for (int i = 0; i < n_features; ++i) {
        XtX_host[i * n_features + i] += lambda;
    }

    CUDA_CHECK(cudaMemcpy(d_regularized, XtX_host.data(), n_features * n_features * sizeof(double),
                         cudaMemcpyHostToDevice));

    // Solve using cuSOLVER (Cholesky decomposition for positive definite matrix)
    int *d_info;
    CUDA_CHECK(cudaMalloc(&d_info, sizeof(int)));

    // API changed in CUDA 12.x - uses newer cusolverDn API
    #if CUDA_VERSION >= 12000
    // CUDA 12.x new API
    size_t workspaceInBytesOnDevice = 0;
    size_t workspaceInBytesOnHost = 0;

    cusolverDnParams_t params;
    cusolverDnCreateParams(&params);

    cusolverDnXpotrf_bufferSize(
        pImpl->cusolver_handle,
        params,
        CUBLAS_FILL_MODE_UPPER,
        n_features,
        CUDA_R_64F,
        d_regularized,
        n_features,
        CUDA_R_64F,
        &workspaceInBytesOnDevice,
        &workspaceInBytesOnHost
    );

    void *d_work = nullptr;
    void *h_work = nullptr;
    if (workspaceInBytesOnDevice > 0) {
        CUDA_CHECK(cudaMalloc(&d_work, workspaceInBytesOnDevice));
    }
    if (workspaceInBytesOnHost > 0) {
        h_work = malloc(workspaceInBytesOnHost);
    }

    // Cholesky factorization (new API)
    cusolverDnXpotrf(
        pImpl->cusolver_handle,
        params,
        CUBLAS_FILL_MODE_UPPER,
        n_features,
        CUDA_R_64F,
        d_regularized,
        n_features,
        CUDA_R_64F,
        d_work,
        workspaceInBytesOnDevice,
        h_work,
        workspaceInBytesOnHost,
        d_info
    );

    // Solve for weights (new API)
    CUDA_CHECK(cudaMemcpy(d_weights, d_Xty, n_features * sizeof(double), cudaMemcpyDeviceToDevice));
    cusolverDnXpotrs(
        pImpl->cusolver_handle,
        params,
        CUBLAS_FILL_MODE_UPPER,
        n_features,
        1,
        CUDA_R_64F,
        d_regularized,
        n_features,
        CUDA_R_64F,
        d_weights,
        n_features,
        d_info
    );

    cusolverDnDestroyParams(params);
    if (d_work) cudaFree(d_work);
    if (h_work) free(h_work);

    #else
    // CUDA 10.2 and 11.x legacy API
    int lwork = 0;
    cusolverDnDpotrf_bufferSize(pImpl->cusolver_handle, CUBLAS_FILL_MODE_UPPER,
                                n_features, d_regularized, n_features, &lwork);

    double *d_work;
    CUDA_CHECK(cudaMalloc(&d_work, lwork * sizeof(double)));

    // Cholesky factorization
    cusolverDnDpotrf(pImpl->cusolver_handle, CUBLAS_FILL_MODE_UPPER,
                    n_features, d_regularized, n_features, d_work, lwork, d_info);

    // Solve for weights
    CUDA_CHECK(cudaMemcpy(d_weights, d_Xty, n_features * sizeof(double), cudaMemcpyDeviceToDevice));
    cusolverDnDpotrs(pImpl->cusolver_handle, CUBLAS_FILL_MODE_UPPER,
                    n_features, 1, d_regularized, n_features, d_weights, n_features, d_info);

    cudaFree(d_work);
    #endif

    // Copy results back
    weights_out.resize(n_features);
    feature_means_out.resize(n_features);
    feature_stds_out.resize(n_features);

    CUDA_CHECK(cudaMemcpy(weights_out.data(), d_weights, n_features * sizeof(double),
                         cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(feature_means_out.data(), d_means, n_features * sizeof(double),
                         cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(feature_stds_out.data(), d_stds, n_features * sizeof(double),
                         cudaMemcpyDeviceToHost));

    cudaFree(d_X);
    cudaFree(d_y);
    cudaFree(d_means);
    cudaFree(d_stds);
    cudaFree(d_weights);
    cudaFree(d_XtX);
    cudaFree(d_Xty);
    cudaFree(d_regularized);
    cudaFree(d_info);

    return true;
}

bool GPUAccelerator::PredictBatch(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& weights,
    const std::vector<double>& feature_means,
    const std::vector<double>& feature_stds,
    std::vector<double>& predictions_out
) {
    if (!pImpl->is_initialized) return false;
    if (X.empty() || weights.empty()) return false;

    int n_samples = X.size();
    int n_features = X[0].size();

    // flatten and normalize X
    std::vector<double> X_flat(n_samples * n_features);
    for (int i = 0; i < n_samples; ++i) {
        for (int j = 0; j < n_features; ++j) {
            double std_val = feature_stds[j] < 1e-10 ? 1.0 : feature_stds[j];
            X_flat[i * n_features + j] = (X[i][j] - feature_means[j]) / std_val;
        }
    }

    // allocate device memory
    double *d_X, *d_weights, *d_predictions;
    CUDA_CHECK(cudaMalloc(&d_X, n_samples * n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_weights, n_features * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_predictions, n_samples * sizeof(double)));

    // copy to device
    CUDA_CHECK(cudaMemcpy(d_X, X_flat.data(), n_samples * n_features * sizeof(double),
                         cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_weights, weights.data(), n_features * sizeof(double),
                         cudaMemcpyHostToDevice));

    // matrix-vector multiplication using cuBLAS
    double alpha = 1.0, beta = 0.0;
    CUBLAS_CHECK(cublasDgemv(pImpl->cublas_handle, CUBLAS_OP_N,
                            n_samples, n_features,
                            &alpha, d_X, n_samples,
                            d_weights, 1,
                            &beta, d_predictions, 1));

    // copy results back
    predictions_out.resize(n_samples);
    CUDA_CHECK(cudaMemcpy(predictions_out.data(), d_predictions, n_samples * sizeof(double),
                         cudaMemcpyDeviceToHost));

    // free
    cudaFree(d_X);
    cudaFree(d_weights);
    cudaFree(d_predictions);

    return true;
}

bool GPUAccelerator::ComputeStatistics(
    const std::vector<double>& data,
    double& mean_out,
    double& std_dev_out,
    double& min_out,
    double& max_out
) {
    if (!pImpl->is_initialized) return false;
    if (data.empty()) return false;

    int n = data.size();

    // uses here Thrust for efficient parallel statistics
    thrust::device_vector<double> d_data(data.begin(), data.end());

    // Compute sum using thrust::reduce
    double sum = thrust::reduce(d_data.begin(), d_data.end(), 0.0, thrust::plus<double>());
    mean_out = sum / n;

    // Compute sum of squares
    thrust::device_vector<double> d_squared(n);
    thrust::transform(d_data.begin(), d_data.end(), d_squared.begin(),
                     [] __device__ (double x) { return x * x; });
    double sum_sq = thrust::reduce(d_squared.begin(), d_squared.end(), 0.0, thrust::plus<double>());

    double variance = (sum_sq / n) - (mean_out * mean_out);
    std_dev_out = sqrt(variance);

    // Compute min and max
    auto minmax = thrust::minmax_element(d_data.begin(), d_data.end());
    min_out = *minmax.first;
    max_out = *minmax.second;

    return true;
}

double GPUAccelerator::ComputeMedian(std::vector<double>& data) {
    if (data.empty()) return 0.0;

    // Copy to device and sort
    thrust::device_vector<double> d_data(data.begin(), data.end());
    thrust::sort(d_data.begin(), d_data.end());

    size_t n = d_data.size();
    if (n % 2 == 0) {
        return (d_data[n/2 - 1] + d_data[n/2]) / 2.0;
    } else {
        return d_data[n/2];
    }
}

bool GPUAccelerator::GetMemoryInfo(size_t& free_mb, size_t& total_mb) {
    size_t free_bytes, total_bytes;
    cudaError_t err = cudaMemGetInfo(&free_bytes, &total_bytes);

    if (err != cudaSuccess) return false;

    free_mb = free_bytes / (1024 * 1024);
    total_mb = total_bytes / (1024 * 1024);

    return true;
}

void GPUAccelerator::Synchronize() {
    cudaDeviceSynchronize();
}

// my gpu profiler for performance monitoring

GPUProfiler::GPUProfiler(const std::string& name) : operation_name(name) {
    cudaEventCreate(&start_event);
    cudaEventCreate(&stop_event);
    cudaEventRecord(start_event);
}

GPUProfiler::~GPUProfiler() {
    cudaEventRecord(stop_event);
    cudaEventSynchronize(stop_event);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start_event, stop_event);

    std::cout << "[GPU Profiler] " << operation_name
              << " took " << milliseconds << " ms" << std::endl;

    cudaEventDestroy(start_event);
    cudaEventDestroy(stop_event);
}

float GPUProfiler::GetElapsedTime() const {
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start_event, stop_event);
    return milliseconds;
}


// unified mem manager impl

void* UnifiedMemoryManager::Allocate(size_t size) {
    void* ptr = nullptr;
    cudaError_t err = cudaMallocManaged(&ptr, size);

    if (err != cudaSuccess) {
        std::cerr << "Failed to allocate unified memory: "
                  << cudaGetErrorString(err) << std::endl;
        return nullptr;
    }

    return ptr;
}

void UnifiedMemoryManager::Free(void* ptr) {
    if (ptr) {
        cudaFree(ptr);
    }
}

void UnifiedMemoryManager::PrefetchToGPU(void* ptr, size_t size) {
    int device = 0;
    cudaGetDevice(&device);

    #if CUDA_VERSION >= 12000
    // CUDA 12.x has improved prefetch API
    cudaMemPrefetchAsync(ptr, size, device, cudaStreamDefault);
    #else
    // CUDA 10.2 and 11.x
    cudaMemPrefetchAsync(ptr, size, device, 0);
    #endif
}

void UnifiedMemoryManager::PrefetchToCPU(void* ptr, size_t size) {
    #if CUDA_VERSION >= 12000
    // CUDA 12.x uses cudaCpuDeviceId
    cudaMemPrefetchAsync(ptr, size, cudaCpuDeviceId, cudaStreamDefault);
    #else
    // CUDA 10.2 and 11.x
    cudaMemPrefetchAsync(ptr, size, cudaCpuDeviceId, 0);
    #endif
}

// GPU MATRIX impl

GPUMatrix::GPUMatrix(int r, int c) : rows(r), cols(c), d_data(nullptr) {
    cudaMalloc(&d_data, rows * cols * sizeof(double));
}

GPUMatrix::~GPUMatrix() {
    if (d_data) {
        cudaFree(d_data);
    }
}

void GPUMatrix::LoadFromCPU(const std::vector<double>& data) {
    if (data.size() != static_cast<size_t>(rows * cols)) {
        std::cerr << "Size mismatch in LoadFromCPU" << std::endl;
        return;
    }

    cudaMemcpy(d_data, data.data(), rows * cols * sizeof(double),
               cudaMemcpyHostToDevice);
}

void GPUMatrix::CopyToCPU(std::vector<double>& data) const {
    data.resize(rows * cols);
    cudaMemcpy(data.data(), d_data, rows * cols * sizeof(double),
               cudaMemcpyDeviceToHost);
}

bool GPUMatrix::Multiply(const GPUMatrix& A, const GPUMatrix& B, GPUMatrix& C) {
    if (A.cols != B.rows || C.rows != A.rows || C.cols != B.cols) {
        std::cerr << "Matrix dimension mismatch" << std::endl;
        return false;
    }

    cublasHandle_t handle;
    cublasCreate(&handle);

    double alpha = 1.0, beta = 0.0;
    cublasStatus_t status = cublasDgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        A.rows, B.cols, A.cols,
        &alpha,
        A.d_data, A.rows,
        B.d_data, B.rows,
        &beta,
        C.d_data, C.rows
    );

    cublasDestroy(handle);

    return (status == CUBLAS_SUCCESS);
}

GPUMatrix GPUMatrix::Transpose() const {
    GPUMatrix result(cols, rows);

    cublasHandle_t handle;
    cublasCreate(&handle);

    double alpha = 1.0, beta = 0.0;
    cublasDgeam(handle,
                CUBLAS_OP_T, CUBLAS_OP_N,
                cols, rows,
                &alpha, d_data, rows,
                &beta, nullptr, cols,
                result.d_data, cols);

    cublasDestroy(handle);

    return result;
}

/**
 * CUDA kernel to add scalar to all matrix elements
 * Compatible with all CUDA versions
 */
__global__ void AddScalarKernel(double* data, int n, double scalar) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Grid-stride loop for better performance on modern GPUs
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        data[i] += scalar;
    }
}

void GPUMatrix::AddScalar(double scalar) {
    int n = rows * cols;
    int block_size = 256;

    // Use grid-stride loop - more efficient on CUDA 11.4+
    int grid_size = std::min((n + block_size - 1) / block_size, 65535);

    AddScalarKernel<<<grid_size, block_size>>>(d_data, n, scalar);
    cudaDeviceSynchronize();
}

/**
 * CUDA kernel for element-wise multiplication
 * Optimized for Ampere architecture (compute 8.7)
 */
__global__ void ElementWiseMultiplyKernel(double* a, const double* b, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Grid-stride loop for better performance
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        a[i] *= b[i];
    }
}

void GPUMatrix::ElementWiseMultiply(const GPUMatrix& other) {
    if (rows != other.rows || cols != other.cols) {
        std::cerr << "Size mismatch in ElementWiseMultiply" << std::endl;
        return;
    }

    int n = rows * cols;
    int block_size = 256;

    // Use grid-stride loop - more efficient on modern GPUs
    int grid_size = std::min((n + block_size - 1) / block_size, 65535);

    ElementWiseMultiplyKernel<<<grid_size, block_size>>>(d_data, other.d_data, n);
    cudaDeviceSynchronize();
}

/**
 * Optimized kernel for computing moving averages
 * Uses shared memory for better performance on Jetson Nano
 */
__global__ void ComputeMovingAverageKernel(
    const double* input,
    double* output,
    int n,
    int window_size
) {
    __shared__ double shared_data[512];

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;

    // Load data into shared memory
    if (idx < n) {
        shared_data[tid] = input[idx];
    } else {
        shared_data[tid] = 0.0;
    }

    __syncthreads();

    // Compute moving average
    if (idx >= window_size - 1 && idx < n) {
        double sum = 0.0;
        for (int i = 0; i < window_size; ++i) {
            int offset = tid - i;
            if (offset >= 0) {
                sum += shared_data[offset];
            } else if (blockIdx.x > 0) {
                // Would need to load from previous block - for simplicity, use global memory
                sum += input[idx - i];
            }
        }
        output[idx] = sum / window_size;
    }
}

/**
 * Kernel for computing exponential moving average (EMA)
 * Used in time series prediction
 */
__global__ void ComputeEMAKernel(
    const double* input,
    double* output,
    int n,
    double alpha
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx == 0 && n > 0) {
        output[0] = input[0];
    } else if (idx > 0 && idx < n) {
        // EMA[t] = alpha * input[t] + (1 - alpha) * EMA[t-1]
        output[idx] = alpha * input[idx] + (1.0 - alpha) * output[idx - 1];
    }
}

/**
 * Kernel to detect anomalies in medication usage
 * Flags values outside mean ± threshold * std_dev
 */
__global__ void DetectAnomaliesKernel(
    const double* data,
    bool* anomalies,
    int n,
    double mean,
    double std_dev,
    double threshold
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n) {
        double z_score = fabs((data[idx] - mean) / std_dev);
        anomalies[idx] = (z_score > threshold);
    }
}

/**
 * Kernel for parallel adherence rate calculation
 * Counts taken vs returned medications
 */
__global__ void ComputeAdherenceRateKernel(
    const bool* is_returns,
    int n,
    int* taken_count,
    int* return_count
) {
    __shared__ int s_taken[256];
    __shared__ int s_returns[256];

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;

    // Initialize shared memory
    s_taken[tid] = 0;
    s_returns[tid] = 0;

    // Count in this thread
    if (idx < n) {
        if (is_returns[idx]) {
            s_returns[tid] = 1;
        } else {
            s_taken[tid] = 1;
        }
    }

    __syncthreads();

    // Reduction in shared memory
    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (tid < stride) {
            s_taken[tid] += s_taken[tid + stride];
            s_returns[tid] += s_returns[tid + stride];
        }
        __syncthreads();
    }

    // Write block results
    if (tid == 0) {
        atomicAdd(taken_count, s_taken[0]);
        atomicAdd(return_count, s_returns[0]);
    }
}

/**
 * Kernel for temporal pattern detection
 * Computes autocorrelation for periodicity detection
 */
__global__ void ComputeAutocorrelationKernel(
    const double* data,
    double* autocorr,
    int n,
    int max_lag
) {
    int lag = blockIdx.x * blockDim.x + threadIdx.x;

    if (lag >= max_lag) return;

    // Compute mean
    double mean = 0.0;
    for (int i = 0; i < n; ++i) {
        mean += data[i];
    }
    mean /= n;

    // Compute autocorrelation for this lag
    double numerator = 0.0;
    double denominator = 0.0;

    for (int i = 0; i < n - lag; ++i) {
        numerator += (data[i] - mean) * (data[i + lag] - mean);
    }

    for (int i = 0; i < n; ++i) {
        denominator += (data[i] - mean) * (data[i] - mean);
    }

    autocorr[lag] = (denominator > 0) ? (numerator / denominator) : 0.0;
}

//GPU FUNCTIONS FOR AI PREDICTIONS
namespace {

/**
 * Helper class for managing GPU streams for concurrent operations
 * Allows overlapping computation and memory transfers
 */
class StreamManager {
public:
    StreamManager(int num_streams) {
        streams.resize(num_streams);
        for (int i = 0; i < num_streams; ++i) {
            cudaStreamCreate(&streams[i]);
        }
    }

    ~StreamManager() {
        for (auto& stream : streams) {
            cudaStreamDestroy(stream);
        }
    }

    cudaStream_t GetStream(int idx) {
        return streams[idx % streams.size()];
    }

    void SynchronizeAll() {
        for (auto& stream : streams) {
            cudaStreamSynchronize(stream);
        }
    }

private:
    std::vector<cudaStream_t> streams;
};

} // anonymous namespace

/**
 * batch prediction with streaming for large datasets
 * Processes data in chunks to handle datasets larger than GPU memory
 */
bool GPUAccelerator::PredictBatchStreaming(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& weights,
    const std::vector<double>& feature_means,
    const std::vector<double>& feature_stds,
    std::vector<double>& predictions_out,
    int chunk_size
) {
    if (!pImpl->is_initialized) return false;
    if (X.empty() || weights.empty()) return false;

    int n_samples = X.size();
    int n_features = X[0].size();

    predictions_out.resize(n_samples);

    // Create streams for concurrent operations
    const int NUM_STREAMS = 4;
    StreamManager stream_mgr(NUM_STREAMS);

    // Process in chunks
    for (int offset = 0; offset < n_samples; offset += chunk_size) {
        int current_chunk = std::min(chunk_size, n_samples - offset);
        int stream_idx = (offset / chunk_size) % NUM_STREAMS;
        cudaStream_t stream = stream_mgr.GetStream(stream_idx);

        // Prepare chunk
        std::vector<double> X_chunk_flat(current_chunk * n_features);
        for (int i = 0; i < current_chunk; ++i) {
            for (int j = 0; j < n_features; ++j) {
                double std_val = feature_stds[j] < 1e-10 ? 1.0 : feature_stds[j];
                X_chunk_flat[i * n_features + j] =
                    (X[offset + i][j] - feature_means[j]) / std_val;
            }
        }

        // Allocate device memory for this chunk
        double *d_X_chunk, *d_weights_chunk, *d_pred_chunk;
        cudaMalloc(&d_X_chunk, current_chunk * n_features * sizeof(double));
        cudaMalloc(&d_weights_chunk, n_features * sizeof(double));
        cudaMalloc(&d_pred_chunk, current_chunk * sizeof(double));

        // Async copy to device
        cudaMemcpyAsync(d_X_chunk, X_chunk_flat.data(),
                       current_chunk * n_features * sizeof(double),
                       cudaMemcpyHostToDevice, stream);
        cudaMemcpyAsync(d_weights_chunk, weights.data(),
                       n_features * sizeof(double),
                       cudaMemcpyHostToDevice, stream);

        // Compute predictions using cuBLAS
        cublasSetStream(pImpl->cublas_handle, stream);
        double alpha = 1.0, beta = 0.0;
        cublasDgemv(pImpl->cublas_handle, CUBLAS_OP_N,
                   current_chunk, n_features,
                   &alpha, d_X_chunk, current_chunk,
                   d_weights_chunk, 1,
                   &beta, d_pred_chunk, 1);

        // Async copy results back
        cudaMemcpyAsync(predictions_out.data() + offset, d_pred_chunk,
                       current_chunk * sizeof(double),
                       cudaMemcpyDeviceToHost, stream);

        // Cleanup (will happen after stream completes)
        cudaStreamSynchronize(stream);
        cudaFree(d_X_chunk);
        cudaFree(d_weights_chunk);
        cudaFree(d_pred_chunk);
    }

    // Wait for all streams to complete
    stream_mgr.SynchronizeAll();

    return true;
}

/**
 * GPU-accelerated cross-validation for model selection
 * Tests different hyperparameters in parallel
 */
bool GPUAccelerator::CrossValidateRidgeRegression(
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& y,
    const std::vector<double>& lambda_values,
    double& best_lambda_out,
    double& best_score_out
) {
    if (!pImpl->is_initialized || X.empty() || y.empty()) return false;

    int n_samples = X.size();
    int k_folds = 5;
    int fold_size = n_samples / k_folds;

    best_score_out = 1e10; // Start with high error
    best_lambda_out = lambda_values[0];

    // Try each lambda value
    for (double lambda : lambda_values) {
        double total_error = 0.0;

        // K-fold cross-validation
        for (int fold = 0; fold < k_folds; ++fold) {
            // Split data into train and validation
            std::vector<std::vector<double>> X_train, X_val;
            std::vector<double> y_train, y_val;

            for (int i = 0; i < n_samples; ++i) {
                if (i >= fold * fold_size && i < (fold + 1) * fold_size) {
                    X_val.push_back(X[i]);
                    y_val.push_back(y[i]);
                } else {
                    X_train.push_back(X[i]);
                    y_train.push_back(y[i]);
                }
            }

            // Train on GPU
            std::vector<double> weights, means, stds;
            TrainRidgeRegression(X_train, y_train, lambda, weights, means, stds);

            // Predict on validation set
            std::vector<double> predictions;
            PredictBatch(X_val, weights, means, stds, predictions);

            // Compute MSE
            double mse = 0.0;
            for (size_t i = 0; i < predictions.size(); ++i) {
                double error = predictions[i] - y_val[i];
                mse += error * error;
            }
            mse /= predictions.size();
            total_error += mse;
        }

        double avg_error = total_error / k_folds;

        if (avg_error < best_score_out) {
            best_score_out = avg_error;
            best_lambda_out = lambda;
        }
    }

    return true;
}

} // namespace CUDA
} // namespace MedicineAI2 - 1] + d_data[n/2]) / 2.0;
    } else {
        return d_data[n/2];
    }
}

bool GPUAccelerator::GetMemoryInfo(size_t& free_mb, size_t& total_mb) {
    size_t free_bytes, total_bytes;
    cudaError_t err = cudaMemGetInfo(&free_bytes, &total_bytes);

    if (err != cudaSuccess) return false;

    free_mb = free_bytes / (1024 * 1024);
    total_mb = total_bytes / (1024 * 1024);

    return true;
}

void GPUAccelerator::Synchronize() {
    cudaDeviceSynchronize();
}


// GPU PROFILER impl

GPUProfiler::GPUProfiler(const std::string& name) : operation_name(name) {
    cudaEventCreate(&start_event);
    cudaEventCreate(&stop_event);
    cudaEventRecord(start_event);
}

GPUProfiler::~GPUProfiler() {
    cudaEventRecord(stop_event);
    cudaEventSynchronize(stop_event);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start_event, stop_event);

    std::cout << "[GPU Profiler] " << operation_name
              << " took " << milliseconds << " ms" << std::endl;

    cudaEventDestroy(start_event);
    cudaEventDestroy(stop_event);
}

float GPUProfiler::GetElapsedTime() const {
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start_event, stop_event);
    return milliseconds;
}


// UNIFIED MEMORY MANAGER IMPLEMENTATION

void* UnifiedMemoryManager::Allocate(size_t size) {
    void* ptr = nullptr;
    cudaError_t err = cudaMallocManaged(&ptr, size);

    if (err != cudaSuccess) {
        std::cerr << "Failed to allocate unified memory: "
                  << cudaGetErrorString(err) << std::endl;
        return nullptr;
    }

    return ptr;
}

void UnifiedMemoryManager::Free(void* ptr) {
    if (ptr) {
        cudaFree(ptr);
    }
}

void UnifiedMemoryManager::PrefetchToGPU(void* ptr, size_t size) {
    int device = 0;
    cudaGetDevice(&device);
    cudaMemPrefetchAsync(ptr, size, device, 0);
}

void UnifiedMemoryManager::PrefetchToCPU(void* ptr, size_t size) {
    cudaMemPrefetchAsync(ptr, size, cudaCpuDeviceId, 0);
}

// GPU MATRIX IMPLEMENTATION

GPUMatrix::GPUMatrix(int r, int c) : rows(r), cols(c), d_data(nullptr) {
    cudaMalloc(&d_data, rows * cols * sizeof(double));
}

GPUMatrix::~GPUMatrix() {
    if (d_data) {
        cudaFree(d_data);
    }
}

void GPUMatrix::LoadFromCPU(const std::vector<double>& data) {
    if (data.size() != static_cast<size_t>(rows * cols)) {
        std::cerr << "Size mismatch in LoadFromCPU" << std::endl;
        return;
    }
    
    cudaMemcpy(d_data, data.data(), rows * cols * sizeof(double),
               cudaMemcpyHostToDevice);
}

void GPUMatrix::CopyToCPU(std::vector<double>& data) const {
    data.resize(rows * cols);
    cudaMemcpy(data.data(), d_data, rows * cols * sizeof(double),
               cudaMemcpyDeviceToHost);
}

bool GPUMatrix::Multiply(const GPUMatrix& A, const GPUMatrix& B, GPUMatrix& C) {
    if (A.cols != B.rows || C.rows != A.rows || C.cols != B.cols) {
        std::cerr << "Matrix dimension mismatch" << std::endl;
        return false;
    }
    
    cublasHandle_t handle;
    cublasCreate(&handle);
    
    double alpha = 1.0, beta = 0.0;
    cublasStatus_t status = cublasDgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        A.rows, B.cols, A.cols,
        &alpha,
        A.d_data, A.rows,
        B.d_data, B.rows,
        &beta,
        C.d_data, C.rows
    );
    
    cublasDestroy(handle);
    
    return (status == CUBLAS_SUCCESS);
}

GPUMatrix GPUMatrix::Transpose() const {
    GPUMatrix result(cols, rows);
    
    cublasHandle_t handle;
    cublasCreate(&handle);
    
    double alpha = 1.0, beta = 0.0;
    cublasDgeam(handle,
                CUBLAS_OP_T, CUBLAS_OP_N,
                cols, rows,
                &alpha, d_data, rows,
                &beta, nullptr, cols,
                result.d_data, cols);
    
    cublasDestroy(handle);
    
    return result;
}

__global__ void AddScalarKernel(double* data, int n, double scalar) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        data[idx] += scalar;
    }
}

void GPUMatrix::AddScalar(double scalar) {
    int n = rows * cols;
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    
    AddScalarKernel<<<grid_size, block_size>>>(d_data, n, scalar);
    cudaDeviceSynchronize();
}

__global__ void ElementWiseMultiplyKernel(double* a, const double* b, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        a[idx] *= b[idx];
    }
}

void GPUMatrix::ElementWiseMultiply(const GPUMatrix& other) {
    if (rows != other.rows || cols != other.cols) {
        std::cerr << "Size mismatch in ElementWiseMultiply" << std::endl;
        return;
    }
    
    int n = rows * cols;
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    
    ElementWiseMultiplyKernel<<<grid_size, block_size>>>(d_data, other.d_data, n);
    cudaDeviceSynchronize();
}

} // namespace CUDA
} // namespace MedicineAI