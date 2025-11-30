cmake_minimum_required(VERSION 3.16)
project(HelloWorld C CXX)

# 设置策略 CMP0144 为 NEW
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置gRPC路径
get_filename_component(GRPC_ROOT "${CMAKE_SOURCE_DIR}/../grpc" ABSOLUTE)
set(CMAKE_PREFIX_PATH "${GRPC_ROOT}")

# 查找依赖包
find_package(Threads REQUIRED)

option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# 添加include路径
set(ALL_INCLUDE_DIRS
"${CMAKE_SOURCE_DIR}/proto")

# 包含子目录
add_subdirectory(proto)
add_subdirectory(server)
add_subdirectory(client)