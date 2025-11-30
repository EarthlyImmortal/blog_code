cmake_minimum_required(VERSION 3.16)
project(server C CXX)

# 可执行文件
add_executable(server server.cpp)

# 添加include文件
foreach(include_dir ${ALL_INCLUDE_DIRS})
    target_include_directories("${PROJECT_NAME}" PRIVATE "${include_dir}")
    message(STATUS "已添加包含路径: ${include_dir}")
endforeach()

target_link_libraries(server 
    hello_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})