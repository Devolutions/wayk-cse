cmake_minimum_required(VERSION 3.0)
project(MockCseDummy C)
add_executable(${PROJECT_NAME} WIN32 "main.c" "resource.rc" "resource.h")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
