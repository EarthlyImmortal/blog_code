// Copyright 2024 gRPC authors.
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

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "helper.h"

#include "hello_world.grpc.pb.h"

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// 服务器行为背后的逻辑和数据。
class GreeterServiceImpl final : public Greeter::CallbackService {
  ServerUnaryReactor* SayHello(CallbackServerContext* context,
                               const HelloRequest* request,
                               HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());

    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(Status::OK);
    return reactor;
  }
};

constexpr char kServerCertPath[] = "../../credentials/localhost.crt";
constexpr char kServerKeyPath[] = "../../credentials/localhost.key";

void RunServer(uint16_t port) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  GreeterServiceImpl service;
  ServerBuilder builder;
  // 加载 SSL 凭据并构建 SSL 凭据选项
  grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {
      LoadStringFromFile(kServerKeyPath), LoadStringFromFile(kServerCertPath)};
  grpc::SslServerCredentialsOptions ssl_options;
  ssl_options.pem_key_cert_pairs.emplace_back(key_cert_pair);
  // 使用 SSL 凭据在给定地址上监听
  builder.AddListeningPort(server_address,
                           grpc::SslServerCredentials(ssl_options));
  // 将“service”注册为我们将通过其与客户端通信的实例。在这种情况下，它对应于一个*同步*服务。
  builder.RegisterService(&service);
  // 最后组装服务器。
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // 等待服务器关闭。注意，必须由其他线程负责关闭服务器，此调用才能返回。
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  RunServer(absl::GetFlag(FLAGS_port));
  return 0;
}
