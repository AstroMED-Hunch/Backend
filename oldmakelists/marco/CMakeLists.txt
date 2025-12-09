cmake_minimum_required(VERSION 3.15)
project(HunchMedical)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(HUNCH_SOURCES
        ../../marco/main.cpp
        ../../marco/modules/aruco/Code.cpp
        ../../marco/Interpreters.cpp
        ../../marco/modules/aruco/Shelf_Bounds.cpp
        ../../marco/mlayout/Lexer.cpp
        ../../marco/mlayout/Parser.cpp
        ../../marco/Module.cpp
        ../../marco/Module.hpp
        ../../marco/Module_Registry.cpp
        ../../marco/Module_Registry.hpp
        ../../marco/modules/aruco/Aruco.cpp
        ../../marco/modules/aruco/Aruco.hpp
        ../../marco/modules/face/Face_Recognition.cpp
        ../../marco/modules/face/Face_Recognition.hpp
)


enable_testing()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("../../marco")

add_executable(HunchMedical ${HUNCH_SOURCES})
target_link_libraries(HunchMedical ${OpenCV_LIBS})


set_property(TARGET HunchMedical PROPERTY COMPILE_WARNING_AS_ERROR ON)