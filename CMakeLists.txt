cmake_minimum_required(VERSION 3.15)
project(NASA_HUNCH_ISS_Medical VERSION 1.0.0 LANGUAGES CXX CUDA)

# Set standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_ARCHITECTURES 53)  # Jetson Nano

# Options
option(ENABLE_CUDA "Enable CUDA acceleration" ON)
option(BUILD_EXAMPLES "Build example programs" ON)

# Find packages
find_package(OpenCV REQUIRED)
find_package(dlib REQUIRED)

if(ENABLE_CUDA)
    find_package(CUDA REQUIRED)
    find_package(CUDAToolkit REQUIRED)
    add_definitions(-DMEDICINE_AI_CUDA_ENABLED)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/mlayout
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/nimul/include
    ${OpenCV_INCLUDE_DIRS}
)

# MARCO'S BASE LIBRARY (Module system + Parser)

set(MARCO_BASE_SOURCES
    marco/Module.cpp
    marco/Module_Registry.cpp
    marco/mlayout/Lexer.cpp
    marco/mlayout/Parser.cpp
    marco/Interpreters.cpp
)

add_library(marco_base STATIC ${MARCO_BASE_SOURCES})
target_link_libraries(marco_base PUBLIC ${OpenCV_LIBS})
target_include_directories(marco_base PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/mlayout
)

# MARCO'S ARUCO MODULE

set(ARUCO_SOURCES
    marco/modules/aruco/Aruco.cpp
    marco/modules/aruco/Code.cpp
    marco/modules/aruco/Shelf_Bounds.cpp
)

add_library(aruco_module STATIC ${ARUCO_SOURCES})
target_link_libraries(aruco_module PUBLIC marco_base ${OpenCV_LIBS})
target_include_directories(aruco_module PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/modules/aruco
)

# MARCO'S FACE RECOGNITION MODULE

set(FACE_SOURCES
    marco/modules/face/Face_Recognition.cpp
)

add_library(face_module STATIC ${FACE_SOURCES})
target_link_libraries(face_module PUBLIC marco_base ${OpenCV_LIBS})
target_include_directories(face_module PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/modules/face
)

# NIMUL - MEDICINE AI LIBRARY

set(MEDICINE_AI_SOURCES
    nimul/src/MedicineAI.cpp
    nimul/src/MedicineAI_C_API.cpp
)

if(ENABLE_CUDA)
    list(APPEND MEDICINE_AI_SOURCES nimul/src/MedicineAI_CUDA.cu)
endif()

add_library(medicine_ai STATIC ${MEDICINE_AI_SOURCES})

target_include_directories(medicine_ai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nimul/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/nimul/src
)

target_link_libraries(medicine_ai
    PUBLIC
        dlib::dlib
    PRIVATE
        ${OpenCV_LIBS}
)

if(ENABLE_CUDA)
    target_link_libraries(medicine_ai
        PUBLIC
            CUDA::cudart
            CUDA::cublas
            CUDA::cusolver
    )
    
    set_target_properties(medicine_ai PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_RESOLVE_DEVICE_SYMBOLS ON
    )
    
    target_compile_options(medicine_ai PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:
            --use_fast_math
            -gencode arch=compute_53,code=sm_53
        >
    )
endif()

# MEDICINE AI MODULE (Integration with Marco's system)

set(MEDICINE_AI_MODULE_SOURCES
    nimul/src/Integration.cpp
)

add_library(medicine_ai_module STATIC ${MEDICINE_AI_MODULE_SOURCES})

target_link_libraries(medicine_ai_module
    PUBLIC
        medicine_ai
        marco_base
    PRIVATE
        ${OpenCV_LIBS}
)

target_include_directories(medicine_ai_module PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/nimul/include
)

# MAIN EXECUTABLE

add_executable(iss_medical_tracker
    marco/main.cpp
)

target_link_libraries(iss_medical_tracker
    PRIVATE
        marco_base
        aruco_module
        face_module
        medicine_ai_module
        ${OpenCV_LIBS}
)

target_include_directories(iss_medical_tracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/marco
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/mlayout
    ${CMAKE_CURRENT_SOURCE_DIR}/marco/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/nimul/include
)

# EXAMPLES (Optional)

if(BUILD_EXAMPLES)
    # Create examples directory if it doesn't exist
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nimul/examples)
    
    # Only build if example files exist
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nimul/examples/example_usage.cpp)
        add_executable(medicine_ai_example
            nimul/examples/example_usage.cpp
        )
        
        target_link_libraries(medicine_ai_example
            PRIVATE
                medicine_ai
                ${OpenCV_LIBS}
        )
    endif()
    
    # CUDA example (if CUDA enabled and file exists)
    if(ENABLE_CUDA AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nimul/examples/cuda_example.cpp)
        add_executable(medicine_ai_cuda_example
            nimul/examples/cuda_example.cpp
        )
        
        target_link_libraries(medicine_ai_cuda_example
            PRIVATE
                medicine_ai
                ${OpenCV_LIBS}
        )
    endif()
endif()

# INSTALLATION

install(TARGETS iss_medical_tracker
    RUNTIME DESTINATION bin
)

# Install libraries
install(TARGETS marco_base aruco_module face_module medicine_ai medicine_ai_module
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY nimul/include/
    DESTINATION include/medicine_ai
    FILES_MATCHING PATTERN "*.hpp"
)

# Create data directory at install
install(DIRECTORY DESTINATION share/iss_medical_tracker/data)

# Print configuration summary
message(STATUS "")
message(STATUS "..")
message(STATUS "NASA HUNCH ISS Medical Tracker")
message(STATUS "...")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CUDA enabled: ${ENABLE_CUDA}")
message(STATUS "Examples: ${BUILD_EXAMPLES}")
message(STATUS "OpenCV: ${OpenCV_VERSION}")
message(STATUS "")
message(STATUS "Source structure:")
message(STATUS "  Marco's code: marco/")
message(STATUS "  MedicineAI: nimul/")
message(STATUS "...")
message(STATUS "")