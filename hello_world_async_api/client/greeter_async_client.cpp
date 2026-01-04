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

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"

#include "hello_world.grpc.pb.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
 public:
  explicit GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // 组装客户端的有效负载，发送给服务器并呈现服务器的响应
  std::string SayHello(const std::string& user) {
    // 要发送到服务器的数据
    HelloRequest request;
    request.set_name(user);

    // 期望从服务器接收的数据容器
    HelloReply reply;

    // 客户端的上下文，可用于向服务器传递额外信息和/或调整某些RPC行为
    ClientContext context;

    // 用于与gRPC运行时异步通信的生产者-消费者队列
    CompletionQueue cq;

    // 存储RPC完成时的状态
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<HelloReply> > rpc(
        stub_->AsyncSayHello(&context, request, &cq));

    // 请求在RPC完成时，用服务器的响应更新"reply"，用操作是否成功的指示更新"status"
    // 使用整数1作为请求的标签
    rpc->Finish(&reply, &status, (void*)1);
    void* got_tag;
    bool ok = false;
    // 阻塞直到完成队列"cq"中有下一个结果可用
    // 应始终检查Next的返回值，该返回值告诉我们是否有任何类型的事件或cq_正在关闭
    CHECK(cq.Next(&got_tag, &ok));

    // 验证来自"cq"的结果是否通过其标签对应于我们之前的请求
    CHECK_EQ(got_tag, (void*)1);
    // ...并验证请求是否成功完成。注意，"ok"仅对应于由Finish()引入的更新请求
    CHECK(ok);

    // 根据实际RPC的状态采取行动
    if (status.ok()) {
      return reply.message();
    } else {
      return "RPC failed";
    }
  }

 private:
  // 从传入的通道中产生存根，存储在此处，这是我们看到的服务器暴露的服务视图
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  // 实例化客户端。它需要一个通道，实际的RPC调用将通过该通道创建
  // 该通道模拟到由"--target="参数指定的端点的连接，这是唯一期望的参数
  std::string target_str = absl::GetFlag(FLAGS_target);
  // 我们指示该通道未经身份验证（使用InsecureChannelCredentials()）
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = greeter.SayHello(user);  // 实际的RPC调用！
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}