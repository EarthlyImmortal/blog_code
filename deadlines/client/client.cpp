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

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_cat.h"
#include "hello_world.grpc.pb.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

void unaryCall(std::shared_ptr<Channel> channel, std::string label,
               std::string message, grpc::StatusCode expected_code)
{
    std::unique_ptr<Greeter::Stub> stub = Greeter::NewStub(channel);

    // 我们将发送给服务器的数据。
    HelloRequest request;
    request.set_name(message);

    // 用于存放来自服务器的数据的容器。
    HelloReply reply;

    // 客户端上下文。可用于向服务器传递额外信息
    // 和/或调整某些 RPC 行为。
    ClientContext context;

    // 设置 1 秒超时
    context.set_deadline(std::chrono::system_clock::now() +
                         std::chrono::seconds(1));

    // 实际的 RPC。
    std::mutex mu;
    std::condition_variable cv;
    bool done = false;
    Status status;
    stub->async()->SayHello(&context, &request, &reply,
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

    // 根据其状态进行响应。
    std::cout << "[" << label << "] wanted = " << expected_code
              << ", got = " << status.error_code() << std::endl;
}

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    // 实例化客户端。它需要一个通道，实际的 RPC
    // 将从该通道创建。该通道模拟到端点的连接，端点由
    // 参数 "--target=" 指定，这是唯一预期的参数。
    std::string target_str = absl::GetFlag(FLAGS_target);
    // 我们指明通道未经过身份验证（使用
    // InsecureChannelCredentials()）。
    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    // 进行测试调用
    unaryCall(channel, "Successful request", "world", grpc::StatusCode::OK);
    unaryCall(channel, "Exceeds deadline", "delay",
              grpc::StatusCode::DEADLINE_EXCEEDED);
    unaryCall(channel, "Successful request with propagated deadline",
              "[propagate me]world", grpc::StatusCode::OK);
    unaryCall(channel, "Exceeds propagated deadline",
              "[propagate me][propagate me]world",
              grpc::StatusCode::DEADLINE_EXCEEDED);
    return 0;
}
