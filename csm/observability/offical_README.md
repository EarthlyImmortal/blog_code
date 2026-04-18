# gRPC C++ CSM Hello World 示例

本 CSM 示例基于 [Hello World 示例](https://github.com/grpc/grpc/tree/master/examples/cpp/helloworld) 修改了 gRPC 客户端和服务器，以测试 CSM 可观测性。

## 配置

客户端接受以下命令行参数：

- target — 默认情况下，客户端尝试连接到 xDS "xds:///helloworld:50051"，gRPC 将使用 xDS 解析此目标并连接到服务器后端。可覆盖此参数以更改目标。
- prometheus_endpoint — 用于 Prometheus 的端点。默认值为 localhost:9464

服务器接受以下命令行参数：

- port — Hello World 服务运行的端口。默认为 50051。
- prometheus_endpoint — 用于 Prometheus 的端点。默认值为 localhost:9464

## 构建

在 gRPC 工作区文件夹中：

客户端：
```
docker build -f examples/cpp/csm/observability/Dockerfile.client
```
服务器：
```
docker build -f examples/cpp/csm/observability/Dockerfile.server
```

要推送到镜像仓库，请通过在上述 `docker build` 命令中添加 `-t` 标志来为镜像添加标签，或运行：

```
docker image tag ${上述构建命令生成的 sha} ${标签}
```

然后使用 `docker push` 推送带标签的镜像。