/*
 *
 * Copyright 2015 gRPC authors.
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

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class CustomHeaderClient
{
   public:
    CustomHeaderClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel))
    {
    }

    // 组装客户端的负载，发送并呈现来自服务器的响应
    std::string SayHello(const std::string& user)
    {
        // 我们将发送给服务器的数据
        HelloRequest request;
        request.set_name(user);

        // 存放我们从服务器期望收到的数据的容器
        HelloReply reply;

        // 客户端上下文。可用于向服务器传递额外信息
        // 和/或调整某些 RPC 行为
        ClientContext context;

        // 设置要发送给服务器的自定义元数据
        context.AddMetadata("custom-header", "Custom Value");

        // 设置自定义二进制元数据
        char bytes[8] = {'\0', '\1', '\2', '\3', '\4', '\5', '\6', '\7'};
        context.AddMetadata("custom-bin", std::string(bytes, 8));

        // 实际的 RPC 调用
        Status status = stub_->SayHello(&context, request, &reply);

        // 根据其状态进行处理
        if (status.ok())
        {
            std::cout << "Client received initial metadata from server: "
                      << context.GetServerInitialMetadata()
                             .find("custom-server-metadata")
                             ->second
                      << std::endl;
            std::cout << "Client received trailing metadata from server: "
                      << context.GetServerTrailingMetadata()
                             .find("custom-trailing-metadata")
                             ->second
                      << std::endl;
            return reply.message();
        }
        else
        {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

   private:
    std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    // 实例化客户端。它需要一个通道，实际的 RPC 调用将基于该通道创建。
    // 该通道模拟了到某个端点（本例中为 localhost:50051）的连接。
    // 我们指明该通道未经过认证（使用了 InsecureChannelCredentials()）。
    CustomHeaderClient greeter(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));
    std::string user("world");
    std::string reply = greeter.SayHello(user);
    std::cout << "Client received message: " << reply << std::endl;
    return 0;
}