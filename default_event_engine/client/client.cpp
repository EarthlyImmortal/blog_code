/*
 *
 * Copyright 2021 gRPC authors.
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
#include "absl/log/log.h"
#include "hello_world.grpc.pb.h"
#include "wrapping_event_engine.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

namespace my_application
{

class GreeterClient
{
   public:
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel))
    {
    }

    std::string SayHello(const std::string& user)
    {
        HelloRequest request;
        request.set_name(user);
        HelloReply reply;
        ClientContext context;
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
        if (status.ok())
        {
            return reply.message();
        }
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
    }

   private:
    std::unique_ptr<Greeter::Stub> stub_;
};

}  // namespace my_application

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    // 创建您选择的某个 EventEngine，很可能是您自己的。
    auto custom_engine =
        std::make_shared<my_application::WrappingEventEngine>();
    // 将此引擎提供给 gRPC。现在对此引擎有两个引用：一个在这里，一个由 gRPC
    // 持有。
    grpc_event_engine::experimental::SetDefaultEventEngine(custom_engine);
    // 此作用域确保在尝试关闭 EventEngine 之前销毁 gRPC 对象。
    {
        std::string target_str = absl::GetFlag(FLAGS_target);
        my_application::GreeterClient greeter(grpc::CreateChannel(
            target_str, grpc::InsecureChannelCredentials()));
        std::string user("EventEngine");
        std::string reply = greeter.SayHello(user);
        std::cout << "Greeter received: " << reply << std::endl;
    }
    LOG(INFO) << "My EventEngine ran " << custom_engine->get_run_count()
              << " closures";
    // 释放应用程序对 EventEngine 的所有权。现在 gRPC 单独拥有该引擎。
    custom_engine.reset();
    // 阻塞直到 gRPC 完成使用该引擎，并且该引擎被销毁。
    grpc_event_engine::experimental::ShutdownDefaultEventEngine();
    return 0;
}
