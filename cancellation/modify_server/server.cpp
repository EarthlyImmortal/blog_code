// Copyright 2023 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "hello_world.grpc.pb.h"

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBidiReactor;
using grpc::ServerBuilder;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// 服务器行为背后的逻辑.
class KeyValueStoreServiceImpl final : public Greeter::CallbackService
{
    ServerBidiReactor<HelloRequest, HelloReply>* SayHelloBidiStream(
        CallbackServerContext* context) override
    {
        class Reactor : public ServerBidiReactor<HelloRequest, HelloReply>
        {
           public:
            explicit Reactor(CallbackServerContext* ctx) : context_(ctx)
            {
                StartRead(&request_);
            }

            void OnReadDone(bool ok) override
            {
                if (!ok)
                {
                    // 客户端取消了rpc
                    if (context_->IsCancelled())
                    {
                        std::cout << "OnReadDone Cancelled!" << std::endl;
                        Finish(grpc::Status::CANCELLED);
                    }
                    else
                    {
                        std::cout << "OnReadDone Finish Read!" << std::endl;
                        Finish(grpc::Status::OK);
                    }
                    return;
                }
                response_.set_message(absl::StrCat(request_.name(), " Ack"));
                StartWrite(&response_);
            }

            void OnWriteDone(bool ok) override
            {
                if (!ok)
                {
                    // 客户端取消了rpc
                    if (context_->IsCancelled())
                    {
                        std::cout << "OnWriteDone Cancelled!" << std::endl;
                        Finish(grpc::Status::CANCELLED);
                    }
                    else
                    {
                        std::cout << "OnWriteDone Failed!" << std::endl;
                        Finish(Status(grpc::StatusCode::UNKNOWN,
                                      "Unexpected Failure"));
                    }
                    return;
                }
                StartRead(&request_);
            }

            void OnDone() override { delete this; }

            void OnCancel() override
            {
                std::cout << "RouteChat RPC Cancelled" << std::endl;
            }

           private:
            CallbackServerContext* context_;
            HelloRequest request_;
            HelloReply response_;
        };

        return new Reactor(context);
    }
};

void RunServer(uint16_t port)
{
    std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
    KeyValueStoreServiceImpl service;

    ServerBuilder builder;
    // 在给定地址上监听，不采用任何认证机制。
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 将 "service" 注册为与客户端通信的实例。这里对应的是一个*同步*服务。
    builder.RegisterService(&service);
    // 最后组装服务器。
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器关闭。注意：必须由其他线程负责关闭服务器，此调用才会返回。
    server->Wait();
}

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    RunServer(absl::GetFlag(FLAGS_port));
    return 0;
}
