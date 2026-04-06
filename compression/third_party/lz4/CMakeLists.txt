cmake_minimum_required(VERSION 3.16)

# 查找当前目录下所有 .c 源文件
message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB LZ4_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
message(STATUS "Found .c files: ${LZ4_SOURCES}")
# 可选：同时把 .h 头文件也加入列表（便于 IDE 显示，但不影响编译）
file(GLOB LZ4_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# 如果找不到任何 .c 文件，报错
if(NOT LZ4_SOURCES)
    message(FATAL_ERROR "No .c source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()


# 确保头文件路径可以被引用
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 创建静态库，链接所有源文件
add_library(${LZ4_LIB_NAME} STATIC
    ${LZ4_SOURCES}
    ${LZ4_HEADERS}      # 头文件加入仅用于 IDE 展示，不影响编译
)