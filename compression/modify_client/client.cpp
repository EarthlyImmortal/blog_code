/*
 *
 * Copyright 2018 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "hello_world.grpc.pb.h"
#include "lz4_helper.h"

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::Status;
using grpc::WriteOptions;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloReplyLz4;
using helloworld::HelloRequest;
using helloworld::HelloRequestLz4;

std::string GenerateWorlds(int count)
{
    std::string result;

    for (int i = 0; i < count; ++i)
    {
        if (i > 0)
        {  // 在非第一个单词前添加空格
            result += " ";
        }
        result += "world";
    }

    return result;
}

class GreeterClient
{
   public:
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel))
    {
    }

    // 组装客户端负载，发送请求并呈现来自服务器的响应。
    std::string SayHello(const std::string& user)
    {
        // 发送到服务器的数据。
        HelloRequest request;
        request.set_name(user);

        // 存放来自服务器预期数据的容器。
        HelloReply reply;

        // 客户端上下文。可用于向服务器传递额外信息和/或调整特定RPC行为。
        ClientContext context;

        // 将调用的压缩算法覆盖为DEFLATE。
        // context.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);

        // 实际RPC调用。
        Status status = stub_->SayHello(&context, request, &reply);

        // 根据状态进行处理。
        if (status.ok())
        {
            return reply.message();
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

    std::string SayHelloLz4(const std::string& user)
    {
        // 1. 压缩请求数据
        int compressed_size = 0;
        std::string user_lz4 = CompressWithLZ4(user, &compressed_size);
        if (user_lz4.empty())
        {
            std::cerr << "Request compression failed" << std::endl;
            return "RPC failed (compress error)";
        }

        HelloRequestLz4 request;
        request.set_name_lz4(user_lz4);
        request.set_original_size(user.size());

        HelloReplyLz4 reply;
        ClientContext context;

        // 2. 调用 RPC
        Status status = stub_->SayHelloLz4(&context, request, &reply);
        if (!status.ok())
        {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }

        // 3. 解压响应数据
        std::string message_lz4 =
            DecompressWithLZ4(reply.message_lz4(), reply.original_size());
        if (message_lz4.empty())
        {
            return "Decompression of response failed";
        }
        return message_lz4;
    }

    void SayHelloBidiStream()
    {
        ClientContext context;

        // 将本地调用的压缩算法设置为gzip
        context.set_compression_algorithm(GRPC_COMPRESS_GZIP);

        // 创建双向流（ClientReaderWriter 同时具备读写能力）
        std::shared_ptr<ClientReaderWriter<HelloRequest, HelloReply>> stream(
            stub_->SayHelloBidiStream(&context));

        // 写线程：客户端发送消息
        std::thread writer_thread(
            [&stream]()
            {
                // 消息1: 正常压缩发送（使用 GZIP）
                {
                    HelloRequest request;
                    request.set_name(
                        "Client msg 1 - COMPRESSED with GZIP "
                        "(normal normal normal normal normal normal normal)");

                    // 默认 WriteOptions，使用 Call 级别设定的 GZIP 压缩
                    stream->Write(request);
                    std::cout << "[Client TX] Sent message 1: COMPRESSED (GZIP)"
                              << std::endl;
                }

                // 消息2: 禁用压缩发送 (Per-Message Disable)
                {
                    HelloRequest request;
                    request.set_name(
                        "Client msg 2 - UNCOMPRESSED (sensitive: "
                        "token=abc123xyz) "
                        "(secret secret secret secret secret secret secret)");

                    // 关键: set_no_compression()
                    // 即使 Call 级别设置了 GZIP，这条消息也不会被压缩
                    // 用于防范 CRIME/BEAST 攻击（保护敏感数据）
                    WriteOptions options;
                    options.set_no_compression();

                    stream->Write(request, options);
                    std::cout << "[Client TX] Sent message 2: UNCOMPRESSED "
                                 "(per-message disable)"
                              << std::endl;
                }

                // 消息3: 恢复正常压缩发送
                {
                    HelloRequest request;
                    request.set_name(
                        "Client msg 3 - COMPRESSED again with GZIP "
                        "(resume resume resume resume resume resume resume)");

                    // 不设置 set_no_compression()，恢复使用 GZIP 压缩
                    stream->Write(request);
                    std::cout << "[Client TX] Sent message 3: COMPRESSED (GZIP)"
                              << std::endl;
                }

                // 消息4: 再次禁用压缩（使用 WriteLast 发送最后一条）
                {
                    HelloRequest request;
                    request.set_name(
                        "Client msg 4 - UNCOMPRESSED & LAST (auth credentials) "
                        "(private private private private private private "
                        "private)");

                    // WriteLast = Write + WritesDone 的合并操作
                    // 同时设置 set_no_compression() 禁用压缩
                    WriteOptions options;
                    options.set_no_compression();

                    stream->WriteLast(request, options);
                    std::cout
                        << "[Client TX] Sent message 4: UNCOMPRESSED & LAST "
                           "(WriteLast)"
                        << std::endl;
                }

                // 注意: 使用 WriteLast 后不需要再调用 WritesDone()
                // 如果使用 Write() 发送最后一条，则需要:
                // stream->WritesDone();
            });

        // 主线程：客户端读取服务端响应
        HelloReply reply;
        while (stream->Read(&reply))
        {
            std::cout << "[Client RX] Received from server: " << reply.message()
                      << std::endl;
        }

        // 等待写线程完成
        writer_thread.join();

        // 获取最终状态
        Status status = stream->Finish();
        if (status.ok())
        {
            std::cout << "[Client] Bidi stream RPC succeeded." << std::endl;
        }
        else
        {
            std::cout << "[Client] Bidi stream RPC failed: "
                      << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

   private:
    std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    // 实例化客户端。需要提供一个通道，实际RPC通过该通道创建。
    // 该通道模拟到端点的连接（本例中为localhost:50051）。
    // 我们指明通道未经过身份验证（使用InsecureChannelCredentials()）。
    ChannelArguments args;
    // // 设置通道的默认压缩算法。
    // args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
    GreeterClient greeter(grpc::CreateCustomChannel(
        "localhost:50051", grpc::InsecureChannelCredentials(), args));
    // std::string user(GenerateWorlds(1600));
    // std::string reply = greeter.SayHelloLz4(user);
    // std::cout << "Greeter received: " << reply << std::endl;

    greeter.SayHelloBidiStream();

    return 0;
}
