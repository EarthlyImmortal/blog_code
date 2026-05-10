# 元数据示例

## 概述

本示例演示如何在客户端和服务器端添加自定义header，以及如何访问它们。

自定义元数据必须遵循 https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md 中列出的“自定义元数据”格式，但二进制header除外（它们不需要进行 base64 编码）。

### 获取教程源代码
本示例及其他示例的代码位于 `examples` 目录中。请执行以下命令，将此仓库的[最新稳定发布标签](https://github.com/grpc/grpc/releases)克隆到本地：
```sh
$ git clone -b RELEASE_TAG_HERE https://github.com/grpc/grpc
```
将当前目录切换至 examples/cpp/metadata
```sh
$ cd examples/cpp/metadata
```

### 生成 gRPC 代码
要生成客户端和服务器端接口：
```sh
$ make helloworld.grpc.pb.cc helloworld.pb.cc
```
内部会调用 proto 编译器，实际执行的命令如下：
```sh
$ protoc -I ../../protos/ --grpc_out=. --plugin=protoc-gen-grpc=grpc_cpp_plugin ../../protos/helloworld.proto
$ protoc -I ../../protos/ --cpp_out=. ../../protos/helloworld.proto
```
### 试试看！
构建客户端和服务器：

```sh
$ make
```

运行服务器，它将监听 50051 端口：

```sh
$ ./greeter_server
```

运行客户端（在另一个终端中）：

```sh
$ ./greeter_client
```

如果一切顺利，你将在客户端终端中看到：

"Client received initial metadata from server: initial metadata value"
"Client received trailing metadata from server: trailing metadata value"
"Client received message: Hello World"

并且在服务器终端中看到：

"Header key: custom-bin , value: 01234567"
"Header key: custom-header , value: Custom Value"
"Header key: user-agent , value: grpc-c++/1.16.0-dev grpc-c/6.0.0-dev (linux; chttp2; gao)"

我们并没有将 user-agent 元数据作为自定义header添加。这展示了 gRPC 框架如何在底层添加一些header，这些header可能会出现在元数据映射中。