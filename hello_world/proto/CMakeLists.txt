# 生成gRPC相关文件
get_filename_component(hello_proto "${CMAKE_CURRENT_SOURCE_DIR}/hello_world.proto" ABSOLUTE)
get_filename_component(hello_proto_path "${hello_proto}" PATH)

# 生成的文件路径
set(hello_proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/hello_world.pb.cc")
set(hello_proto_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/hello_world.pb.h")
set(hello_grpc_srcs "${CMAKE_CURRENT_SOURCE_DIR}/hello_world.grpc.pb.cc")
set(hello_grpc_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/hello_world.grpc.pb.h")

# 生成命令
add_custom_command(
  OUTPUT "${hello_proto_srcs}" "${hello_proto_hdrs}" "${hello_grpc_srcs}" "${hello_grpc_hdrs}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}"
       --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
       -I "${hello_proto_path}"
       --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
       "${hello_proto}"
  DEPENDS "${hello_proto}")

# 创建库
add_library(hello_grpc_proto
  ${hello_grpc_srcs}
  ${hello_grpc_hdrs}
  ${hello_proto_srcs}
  ${hello_proto_hdrs})

target_link_libraries(hello_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})