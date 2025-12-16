# Project Documentation (consists of all of the C API functions by Nimul)

#info on source and headers

- **MedicineAI.hpp** - Main API header
- **MedicineAI.cpp** - Core implementation
- **MedicineAI_CUDA.hpp** - GPU acceleration header
- **MedicineAI_CUDA.cu** - GPU implementation
- **MedicineAI_C_API.cpp** - C wrapper
- **Integration.cpp** - Marco's system integration
- **CMakeLists.txt** - Build system with CUDA
- **README.md** - Full documentation
- **JETSON_NANO_GUIDE.md** - Optimization guide
- **example_usage.cpp** - Usage examples

#Header File Includes eight Structures:

- **MedicationForm enum** - Tablet, capsule, liquid, and more.
- **Medication struct** - Full medication details
- **AdherenceLog struct** - Who, what, when tracking
- **PredictionResult struct** - AI prediction outputs
- **UserStatistics struct** - Per-user analytics
- **SystemStatistics struct** - System-wide analytics
- **ResupplyRecommendation struct** - Single resupply item
- **ResupplyReport struct** - Complete ISS shipment report
- **ExternalDataPacket struct** - External integration data

#Main MedicineTracker Class has 40+ methods and I also included over 25 C API functions:
Contains 40+ methods and over 25 C API functions.

### Configuration
- `LoadConfiguration()` - Load from Marco's system
- `SetConfigValue()` / `GetConfigValue()`

### Medication Database Methods
- `AddMedication()` / `UpdateMedication()` / `RemoveMedication()`
- `GetMedication()` / `GetAllMedications()`
- `SearchMedications()` - Search by name

### Adherence Logging
- `LogPickup()` / `LogReturn()` / `LogAdherence()`
- `GetUserAdherenceHistory()` / `GetMedicationAdherenceHistory()`
- `GetAdherenceHistoryInRange()`

### Statistics & Analytics
- `GetUserStatistics()` / `GetSystemStatistics()`
- `CalculateAdherenceRate()`
- `GetActiveUsers()` / `GetActiveMedications()`

### AI Predictions
- `PredictUsage()` - Main prediction function
- `GenerateResupplyReport()` - ISS resupply calculations
- `TrainModels()` - Train/retrain AI
- `GetModelInfo()` - Model status

### Data Persistence
- `SaveToStorage()` / `LoadFromStorage()`
- `ExportToCSV()` / `ImportFromCSV()`
- `ClearAllLogs()` - Clear all data
- `CreateBackup()` / `RestoreFromBackup()`

#NIMUL'S CUDA / GPU ACCELARATOR:

### GPU Accelerator Functions
- `ExtractFeatures()` - Parallel feature extraction
- `TrainRidgeRegression()` - GPU-accelerated training with cuBLAS/cuSOLVER
- `PredictBatch()` - Batch predictions on GPU
- `ComputeStatistics()` - Parallel statistics with Thrust
- `ComputeMedian()` - GPU sorting and median
- `GetMemoryInfo()` - Memory monitoring for Jetson Nano
- `Synchronize()` - GPU synchronization

### GPU Profiler
- Constructor/Destructor with CUDA events
- `GetElapsedTime()` for performance measurement

### Unified Memory Manager Functions
- `Allocate()` - Unified memory allocation
- `Free()` - Memory cleanup
- `PrefetchToGPU()` - Optimize for GPU access
- `PrefetchToCPU()` - Optimize for CPU access

### GPU Matrix Class
- Constructor/Destructor
- `LoadFromCPU()` / `CopyToCPU()` - Data transfer
- `Multiply()` - Matrix multiplication with cuBLAS
- `Transpose()` - Matrix transpose
- `AddScalar()` - Element-wise scalar addition
- `ElementWiseMultiply()` - Element-wise multiplication

#some cool features for troubleshooting and crap:
- `PredictBatchStreaming()` - Handle datasets larger than GPU memory
- `CrossValidateRidgeRegression()` - GPU-accelerated hyperparameter tuning
- **StreamManager** - Concurrent CUDA streams for overlapping operations

#My state of the art CUDA Kernels:
- **ExtractFeaturesKernel** - Parallel feature extraction (15 features)
- **NormalizeFeaturesKernel** - Z-score normalization
- **ComputeMeansKernel** - Feature means
- **ComputeStdDevsKernel** - Feature standard deviations
- **MatVecMultKernel** - Matrix-vector multiplication
- **ComputeStatisticsKernel** - Parallel reduction for statistics
- **ComputeMovingAverageKernel** - Moving average with shared memory
- **ComputeEMAKernel** - Exponential moving average
- **DetectAnomaliesKernel** - Anomaly detection
- **ComputeAdherenceRateKernel** - Parallel adherence calculation
- **ComputeAutocorrelationKernel** - Temporal pattern detection
- **AddScalarKernel** - Scalar addition kernel
- **ElementWiseMultiplyKernel** - Element-wise multiplication kernel

