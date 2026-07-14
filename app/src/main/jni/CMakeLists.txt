# CMake 配置 for APEX Root Detection

cmake_minimum_required(VERSION 3.18.1)

project("apexroot")

# 启用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Android log 库
find_library(log-lib log)

# 添加原生库
add_library(apexroot SHARED native-lib.cpp)

# 链接库
target_link_libraries(apexroot ${log-lib} android)

# 编译选项
target_compile_options(apexroot PRIVATE
    -Wall
    -Wextra
    -fvisibility=hidden
    -fvisibility-inlines-hidden
)
