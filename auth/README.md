# 认证示例

## 概述

SSL是一种常用的加密协议，用于提供端到端的通信安全。在本示例中，我们演示如何设置一个服务器认证的SSL连接来传输RPC。

我们提供了 `grpc::SslServerCredentials` 和 `grpc::SslCredentials` 类型来使用SSL连接。

在我们的示例中，我们使用提前创建的公钥/私钥：
* "localhost.crt" 包含服务器证书（公钥）。
* "localhost.key" 包含服务器私钥。
* "root.crt" 包含可以验证服务器证书的证书（证书颁发机构）。

### 尝试运行！

一旦你有了可用的gRPC，你可以使用bazel或cmake构建这个示例。请确保在此目录下运行它们，以便能够正确读取凭证文件。

运行服务器，它将在端口50051上监听：

```sh
$ ./ssl_server
```

运行客户端（在另一个终端中）：

```sh
$ ./ssl_client
```

如果一切顺利，你将看到客户端输出：

```
Greeter received: Hello world
```