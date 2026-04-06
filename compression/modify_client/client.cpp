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

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "hello_world.grpc.pb.h"
#include "lz4_helper.h"

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
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
    std::string user(GenerateWorlds(1600));
    std::string reply = greeter.SayHelloLz4(user);
    std::cout << "Greeter received: " << reply << std::endl;

    return 0;
}
