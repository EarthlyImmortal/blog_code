# gRPC C++ CSM Hello World 示例

本 CSM 示例基于 [Hello World 示例](https://github.com/grpc/grpc/tree/master/examples/cpp/helloworld) 开发，修改了 gRPC 客户端和服务器，使其能够接受来自 xDS 控制平面的配置，并测试 SSA 和 CSM 可观测性。

## 配置

客户端接受以下命令行参数：

- target — 默认情况下，客户端尝试连接到 xDS 地址 "xds:///helloworld:50051"，gRPC 将使用 xDS 解析该目标并连接到服务器后端。可以通过覆盖该参数来更改目标地址。
- cookie_name — 会话亲和性（session affinity）Cookie 名称。默认值为 "GSSA"。
- delay_s — RPC 之间的延迟时间（秒）。默认值为 5。

服务器接受以下命令行参数：

- port — Hello World 服务的运行端口。默认值为 50051。

## 构建

在 gRPC 工作区目录下执行：

客户端：
```
docker build -f examples/cpp/csm/Dockerfile.client
```
服务器：
```
docker build -f examples/cpp/csm/Dockerfile.server
```

如需将镜像推送到镜像仓库，可以在上述 `docker build` 命令中添加 `-t` 标签参数，或者执行以下命令为镜像添加标签：

```
docker image tag ${上述构建命令输出的 sha} ${标签}
```

然后使用 `docker push` 推送带标签的镜像。