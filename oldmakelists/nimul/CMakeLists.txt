cmake_minimum_required(VERSION 3.15)
project(MedicineAI VERSION 1.0.0 LANGUAGES CXX CUDA)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set CUDA standard
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# CUDA architecture for Jetson Nano (Maxwell - Compute Capability 5.3)
set(CMAKE_CUDA_ARCHITECTURES 53)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(dlib REQUIRED)
find_package(CUDA REQUIRED)
find_package(CUDAToolkit REQUIRED)

# Option to build as standalone or as part of Marco's project
option(BUILD_STANDALONE "Build as standalone library" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test suite" OFF)
option(ENABLE_CUDA "Enable CUDA acceleration for Jetson Nano" ON)

# MEDICINE AI LIBRARY

# Source files for the main library
set(MEDICINE_AI_SOURCES
        ../../../NimulMedAI/src/MedicineAI.cpp
        ../../../NimulMedAI/src/MedicineAI_C_API.cpp
)

# CUDA sources (if enabled)
if(ENABLE_CUDA)
    list(APPEND MEDICINE_AI_SOURCES ../../../NimulMedAI/src/MedicineAI_CUDA.cu)
    add_definitions(-DMEDICINE_AI_CUDA_ENABLED)
endif()

# Header files
set(MEDICINE_AI_HEADERS
        ../../nimul/include/MedicineAI.hpp
)

if(ENABLE_CUDA)
    list(APPEND MEDICINE_AI_HEADERS ../../nimul/include/MedicineAI_CUDA.hpp)
endif()

# Create the main library
add_library(medicine_ai STATIC ${MEDICINE_AI_SOURCES})

# Include directories
target_include_directories(medicine_ai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ../../../NimulMedAI/src
)

# Link dependencies
target_link_libraries(medicine_ai
    PUBLIC
        dlib::dlib
    PRIVATE
        ${OpenCV_LIBS}
)

# Add CUDA libraries if enabled
if(ENABLE_CUDA)
    target_link_libraries(medicine_ai
        PUBLIC
            CUDA::cudart
            CUDA::cublas
            CUDA::cusolver
    )
    
    # Set CUDA-specific properties
    set_target_properties(medicine_ai PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
    
    # Jetson Nano optimizations
    target_compile_options(medicine_ai PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:
            --use_fast_math
            --ptxas-options=-v
            -gencode arch=compute_53,code=sm_53
        >
    )
endif()

# Compiler options
target_compile_options(medicine_ai PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# INTEGRATION MODULE (for Marco's system)

if(NOT BUILD_STANDALONE)
    # Integration module requires Marco's base classes
    set(INTEGRATION_SOURCES
            ../../../NimulMedAI/src/Integration.cpp
    )
    
    add_library(medicine_ai_module STATIC ${INTEGRATION_SOURCES})
    
    target_link_libraries(medicine_ai_module
        PUBLIC
            medicine_ai
        PRIVATE
            marco_modules  # Marco's module system
    )
    
    target_include_directories(medicine_ai_module
        PUBLIC
            ../../../NimulMedAI/include
    )
endif()

# EXAMPLES

if(BUILD_EXAMPLES)
    add_executable(medicine_ai_examples
        examples/example_usage.cpp
    )
    
    target_link_libraries(medicine_ai_examples
        PRIVATE
            medicine_ai
            ${OpenCV_LIBS}
    )
    
    # Simple C API example
    add_executable(c_api_example
        examples/c_api_simple.c
    )
    
    target_link_libraries(c_api_example
        PRIVATE
            medicine_ai
    )
    
    # Integration example (only if not standalone)
    if(NOT BUILD_STANDALONE)
        add_executable(marco_integration_example
            examples/marco_integration_example.cpp
        )
        
        target_link_libraries(marco_integration_example
            PRIVATE
                medicine_ai_module
                marco_modules
                ${OpenCV_LIBS}
        )
    endif()
endif()

# TESTS

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(medicine_ai_tests
        tests/test_medication_database.cpp
        tests/test_adherence_logging.cpp
        tests/test_predictions.cpp
        tests/test_statistics.cpp
        tests/test_resupply.cpp
    )
    
    target_link_libraries(medicine_ai_tests
        PRIVATE
            medicine_ai
            GTest::GTest
            GTest::Main
    )
    
    add_test(NAME MedicineAITests COMMAND medicine_ai_tests)
endif()

# INSTALLATION

# Install library
install(TARGETS medicine_ai
    EXPORT MedicineAITargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ../../../NimulMedAI/include
)

# Install headers
install(FILES ${MEDICINE_AI_HEADERS}
    DESTINATION include/medicine_ai
)

# Install integration module if built
if(NOT BUILD_STANDALONE)
    install(TARGETS medicine_ai_module
        EXPORT MedicineAITargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

# Export targets
install(EXPORT MedicineAITargets
    FILE MedicineAITargets.cmake
    NAMESPACE MedicineAI::
    DESTINATION lib/cmake/MedicineAI
)

# Create and install Config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MedicineAIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(cmake/MedicineAIConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/MedicineAIConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MedicineAIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MedicineAIConfigVersion.cmake"
    DESTINATION lib/cmake/MedicineAI
)


# DOCUMENTATION


option(BUILD_DOCUMENTATION "Build API documentation" OFF)

if(BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(doc_medicine_ai ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# SUMMARY
message(STATUS "")
message(STATUS "...")
message(STATUS "MedicineAI Configuration Summary")
message(STATUS "...")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(ENABLE_CUDA)
    message(STATUS "CUDA: Enabled (Jetson Nano Optimized)")
    message(STATUS "CUDA Architectures: ${CMAKE_CUDA_ARCHITECTURES}")
    message(STATUS "CUDA Toolkit: ${CUDAToolkit_VERSION}")
else()
    message(STATUS "CUDA: Disabled")
endif()
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Standalone: ${BUILD_STANDALONE}")
message(STATUS "  Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Tests: ${BUILD_TESTS}")
message(STATUS "  Documentation: ${BUILD_DOCUMENTATION}")
message(STATUS "  CUDA Acceleration: ${ENABLE_CUDA}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  OpenCV: ${OpenCV_VERSION}")
message(STATUS "  dlib: Found")
if(ENABLE_CUDA)
    message(STATUS "  CUDA: ${CUDA_VERSION}")
    message(STATUS "  cuBLAS: Found")
    message(STATUS "  cuSOLVER: Found")
endif()
message(STATUS "...")
message(STATUS "")