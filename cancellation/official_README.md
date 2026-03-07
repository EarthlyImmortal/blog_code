# 取消操作示例

## 概述

本例展示了如何从客户端发起取消操作，以及如何在服务端和客户端获知取消事件。

### 动手尝试！

在准备好可运行的 gRPC 环境后，你可以使用 bazel 或 cmake 构建此示例。

运行服务端，它将监听 50051 端口：

```sh
$ ./server
```

（在另一个终端中）运行客户端：

```sh
$ ./client
```

一切顺利的话，你将看到客户端输出：

```
Begin : Begin Ack
Count 1 : Count 1 Ack
Count 2 : Count 2 Ack
Count 3 : Count 3 Ack
Count 4 : Count 4 Ack
Count 5 : Count 5 Ack
Count 6 : Count 6 Ack
Count 7 : Count 7 Ack
Count 8 : Count 8 Ack
Count 9 : Count 9 Ack
RPC Cancelled!
```