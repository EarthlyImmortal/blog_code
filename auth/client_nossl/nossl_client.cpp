/*
 *
 * Copyright 2024 gRPC authors.
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

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "hello_world.grpc.pb.h"

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;
class GreeterClient
{
   public:
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel))
    {
    }

    // 组装客户端的负载，发送它，并呈现来自服务器的响应。
    std::string SayHello(const std::string& user)
    {
        // 我们发送给服务器的数据。
        HelloRequest request;
        request.set_name(user);

        // 用于存放我们从服务器期望的数据的容器。
        HelloReply reply;

        // 客户端的上下文。它可用于向服务器传递额外信息和/或调整某些 RPC 行为。
        ClientContext context;

        // 实际的 RPC。
        std::mutex mu;
        std::condition_variable cv;
        bool done = false;
        Status status;
        stub_->async()->SayHello(&context, &request, &reply,
                                 [&mu, &cv, &done, &status](Status s)
                                 {
                                     status = std::move(s);
                                     std::lock_guard<std::mutex> lock(mu);
                                     done = true;
                                     cv.notify_one();
                                 });

        std::unique_lock<std::mutex> lock(mu);
        while (!done)
        {
            cv.wait(lock);
        }

        // 根据其状态采取行动。
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

   private:
    std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    // 实例化客户端。它需要一个通道，实际 RPC 由此创建。此通道模拟连接到由参数
    // "--target=" 指定的端点，这是唯一的预期参数。
    std::string target_str =
        absl::StrFormat("localhost:%d", absl::GetFlag(FLAGS_port));
    // 创建通道
    GreeterClient greeter(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    std::string user("world");
    std::string reply = greeter.SayHello(user);
    std::cout << "Greeter received: " << reply << std::endl;

    return 0;
}
