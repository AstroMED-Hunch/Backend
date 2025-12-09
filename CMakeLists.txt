cmake_minimum_required(VERSION 3.15)
project(HunchMedical)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(HUNCH_SOURCES
    src/main.cpp
    src/modules/aruco/Code.cpp
    src/Interpreters.cpp
    src/modules/aruco/Shelf_Bounds.cpp
    src/mlayout/Lexer.cpp
    src/mlayout/Parser.cpp
    src/Module.cpp
    src/Module.hpp
    src/Module_Registry.cpp
    src/Module_Registry.hpp
    src/modules/aruco/Aruco.cpp
    src/modules/aruco/Aruco.hpp
    src/modules/face/Face_Recognition.cpp
    src/modules/face/Face_Recognition.hpp
)


enable_testing()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("./src/")

add_executable(HunchMedical ${HUNCH_SOURCES})
target_link_libraries(HunchMedical ${OpenCV_LIBS})


set_property(TARGET HunchMedical PROPERTY COMPILE_WARNING_AS_ERROR ON)