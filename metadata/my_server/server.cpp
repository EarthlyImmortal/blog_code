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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// 服务器行为背后的逻辑和数据。
class GreeterServiceImpl final : public Greeter::Service
{
    Status SayHello(ServerContext* context, const HelloRequest* request,
                    HelloReply* reply) override
    {
        std::string prefix("Hello ");

        // 获取客户端的初始元数据
        std::cout << "Client metadata: " << std::endl;
        const std::multimap<grpc::string_ref, grpc::string_ref> metadata =
            context->client_metadata();
        for (auto iter = metadata.begin(); iter != metadata.end(); ++iter)
        {
            std::cout << "Header key: " << iter->first << ", value: ";
            // 检查是否为二进制值
            size_t isbin = iter->first.find("-bin");
            if ((isbin != std::string::npos) &&
                (isbin + 4 == iter->first.size()))
            {
                std::cout << std::hex;
                for (auto c : iter->second)
                {
                    std::cout << static_cast<unsigned int>(c);
                }
                std::cout << std::dec;
            }
            else
            {
                std::cout << iter->second;
            }
            std::cout << std::endl;
        }

        // 测试请求头大小超上限
        const int metadata_num = 200;
        const std::string large_value(100, 'X');  // 100 字节的字符串

        for (int i = 0; i < metadata_num; ++i)
        {
            std::string key = "test-header-" + std::to_string(i);
            // 普通文本元数据（键中不能有 '-bin' 后缀，否则会被当做二进制处理）
            context->AddInitialMetadata(key, large_value);
        }

        for (int i = 0; i < metadata_num; ++i)
        {
            std::string key = "test-trailer-" + std::to_string(i);
            // 普通文本元数据（键中不能有 '-bin' 后缀，否则会被当做二进制处理）
            context->AddTrailingMetadata(key, large_value);
        }

        context->AddInitialMetadata("custom-server-metadata",
                                    "initial metadata value");
        context->AddTrailingMetadata("custom-trailing-metadata",
                                     "trailing metadata value");
        reply->set_message(prefix + request->name());
        return Status::OK;
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    GreeterServiceImpl service;

    ServerBuilder builder;
    // 在给定地址上监听，不使用任何认证机制。
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 将“service”注册为用于与客户端通信的实例。
    // 在这种情况下，它对应的是一个*同步*服务。
    builder.RegisterService(&service);

    // 在服务端设置接收 metadata 的大小限制
    const int new_limit = 30 * 1024;
    builder.AddChannelArgument(GRPC_ARG_MAX_METADATA_SIZE, new_limit);

    // 最后组装服务器。
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器关闭。请注意，必须由其他某个线程负责关闭服务器，
    // 此调用才会返回。
    server->Wait();
}

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    RunServer();

    return 0;
}
