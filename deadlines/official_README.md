# Deadline 示例

## 概述

本示例演示了如何在调用时使用截止时间（deadline）。

### 动手试试！

在拥有可用的 gRPC 之后，您可以使用 bazel 或 cmake 构建本示例。

运行服务器，它将监听 50051 端口：

```sh
$ ./server
```

运行客户端（在另一个终端中）：

```sh
$ ./client
```

为了模拟测试场景，测试服务器实现了以下功能：
- 响应延迟：服务器会故意延迟对 `delay` 请求消息的响应，以引发超时情况。
- 截止时间传播：当收到带有 `[propagate me]` 前缀的请求时，服务器会将该请求转发回自身。
  这模拟了系统中截止时间的传播过程。

如果一切顺利，您将看到客户端输出：

```
[Successful request] wanted = 0, got = 0
[Exceeds deadline] wanted = 4, got = 4
[Successful request with propagated deadline] wanted = 0, got = 0
[Exceeds propagated deadline] wanted = 4, got = 4
```