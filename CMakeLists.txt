cmake_minimum_required(VERSION 3.5)
project(vulkan_play)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp VDeleter.cpp VDeleter.h)
add_executable(vulkan_play ${SOURCE_FILES})
target_link_libraries(vulkan_play vulkan glfw)
