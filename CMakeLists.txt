cmake_minimum_required(VERSION 3.10)
project(GameEngine)
set(CMAKE_CPP_STANDARD 17)
set(CMAKE_CPP_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Set your source files
set(SOURCES main.cpp glad.c include/glad/glad.h)

# Add executable and link against OpenGL and GLUT
add_executable(GameEngine ${SOURCES})

target_link_libraries(GameEngine glfw)