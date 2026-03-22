# gRPC C++ 消息压缩教程

### 先决条件
请确保您已经运行过[hello world示例](../helloworld)或理解了gRPC的基础知识。我们将不再深入讨论hello world示例中已涉及的内容。

### 获取教程源代码

本教程及其他示例的代码位于`examples`目录中。通过运行以下命令，将[最新稳定版本标签](https://github.com/grpc/grpc/releases)的仓库克隆到你的本地机器：


```sh
$ git clone -b RELEASE_TAG_HERE https://github.com/grpc/grpc
```

将当前目录更改为 examples/cpp/compression

```sh
$ cd examples/cpp/compression/
```

### 生成gRPC代码

要生成客户端和服务端接口：

```sh
$ make helloworld.grpc.pb.cc helloworld.pb.cc
```
该命令内部调用协议编译器，具体如下：

```sh
$ protoc -I ../../protos/ --grpc_out=. --plugin=protoc-gen-grpc=grpc_cpp_plugin ../../protos/helloworld.proto
$ protoc -I ../../protos/ --cpp_out=. ../../protos/helloworld.proto
```

### 编写客户端和服务端

客户端和服务端可以基于hello world示例。

此外，我们还可以配置压缩设置。

在客户端，通过通道参数设置通道的默认压缩算法。

```cpp
  ChannelArguments args;
  // 设置通道的默认压缩算法。
  args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
  GreeterClient greeter(grpc::CreateCustomChannel(
      "localhost:50051", grpc::InsecureChannelCredentials(), args));
```

每个调用的压缩配置可以通过客户端上下文进行覆盖。

```cpp
    // 将调用的压缩算法覆盖为 DEFLATE。
    context.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
```

在服务端，通过服务端构建器设置默认压缩算法。

```cpp
  ServerBuilder builder;
  // 设置服务端的默认压缩算法。
  builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
```

每个调用的压缩配置可以通过服务端上下文进行覆盖。

```cpp
    // 将调用的压缩算法覆盖为 DEFLATE。
    context->set_compression_algorithm(GRPC_COMPRESS_DEFLATE);
```

如需查看完整示例，请参考[greeter_client.cc](greeter_client.cc)和[greeter_server.cc](greeter_server.cc)。

通过以下命令构建并运行（支持压缩的）客户端和服务端。

```sh
make
./greeter_server
```

```sh
./greeter_client
```