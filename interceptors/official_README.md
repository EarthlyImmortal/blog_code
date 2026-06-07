## gRPC C++ 拦截器示例

C++ 拦截器示例展示了如何将拦截器用于一个简单的键值存储。请注意，C++ 拦截 API 目前仍是实验性的，可能会发生变化。

## 键值存储

键值存储服务的定义在 [keyvaluestore.proto](https://github.com/grpc/grpc/blob/master/examples/protos/keyvaluestore.proto) 文件中。它包含一个简单的双向流式 RPC，其中请求消息包含一个键，响应消息包含对应的值。

该示例展示了一个非常简单的 `CachingInterceptor`（缓存拦截器），它被添加到客户端通道上，用于缓存它看到的键值对。如果客户端查找的键存在于缓存中，拦截器会直接返回缓存的值，服务器不会收到该键的请求。

在服务器端，添加了一个非常简单的日志记录拦截器，它每当收到新的 RPC 时，就会向标准输出打印日志。

## 运行示例

启动服务器：

```
$ tools/bazel run examples/cpp/interceptors:keyvaluestore_server
```

（在另一个终端中）启动客户端：

```
$ tools/bazel run examples/cpp/interceptors:keyvaluestore_client
```