# Set some flags
cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE Debug)

# Create our project for further reference
project(vulkan_play)

set(VK_SOURCE main.cpp vk_deleter.h)
add_executable(vkplay ${VK_SOURCE})

# Library dependencies for the http extention
target_link_libraries(vkplay vulkan glfw)
