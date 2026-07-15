# gRPC C++ Hello World 示例 —— 使用自定义 EventEngine

您可以在 [C++ 快速入门][] 中找到构建 gRPC 并运行 Hello World 应用的完整说明。

此示例演示了如何为 gRPC 提供自定义 [EventEngine][]。
通过为 gRPC 提供应用程序自有的 EventEngine，应用程序可以自定义 gRPC 在执行 I/O、异步回调执行、定时器执行和 DNS 解析时的大部分行为。

[C++ 快速入门]: https://grpc.io/docs/languages/cpp/quickstart
[EventEngine]: https://github.com/grpc/grpc/blob/master/include/grpc/event_engine/event_engine.h