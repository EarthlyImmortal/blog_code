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
#include <grpcpp/support/server_interceptor.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "key_value_store.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBidiReactor;
using grpc::ServerBuilder;
using grpc::Status;
using grpc::experimental::InterceptionHookPoints;
using grpc::experimental::Interceptor;
using grpc::experimental::InterceptorBatchMethods;
using grpc::experimental::ServerInterceptorFactoryInterface;
using grpc::experimental::ServerRpcInfo;
using keyvaluestore::KeyValueStore;
using keyvaluestore::Request;
using keyvaluestore::Response;

// 这是一个简单的拦截器，每当收到请求时就会记录日志，
// 在服务端，这发生在收到初始元数据时。
class LoggingInterceptor : public Interceptor
{
   public:
    LoggingInterceptor()
    {
        std::cout << "LoggingInterceptor created!" << std::endl;
    }

    void Intercept(InterceptorBatchMethods* methods) override
    {
        if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_RECV_INITIAL_METADATA))
        {
            std::cout << "Got a new streaming RPC" << std::endl;
        }
        methods->Proceed();
    }

    ~LoggingInterceptor()
    {
        std::cout << "LoggingInterceptor destoryed!" << std::endl;
    }
};

class LoggingInterceptorFactory : public ServerInterceptorFactoryInterface
{
   public:
    Interceptor* CreateServerInterceptor(ServerRpcInfo* info) override
    {
        std::ignore = info;
        return new LoggingInterceptor();
    }
};

struct kv_pair
{
    const char* key;
    const char* value;
};

static const kv_pair kvs_map[] = {
    {"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"},
    {"key4", "value4"}, {"key5", "value5"},
};

const char* get_value_from_map(const char* key)
{
    for (size_t i = 0; i < sizeof(kvs_map) / sizeof(kv_pair); ++i)
    {
        if (strcmp(key, kvs_map[i].key) == 0)
        {
            return kvs_map[i].value;
        }
    }
    return "";
}

// 服务端行为背后的逻辑。
class KeyValueStoreServiceImpl final : public KeyValueStore::CallbackService
{
    ServerBidiReactor<Request, Response>* GetValues(
        CallbackServerContext* context) override
    {
        std::ignore = context;
        class Reactor : public ServerBidiReactor<Request, Response>
        {
           public:
            explicit Reactor() { StartRead(&request_); }

            void OnReadDone(bool ok) override
            {
                if (!ok)
                {
                    return Finish(grpc::Status::CANCELLED);
                }
                response_.set_value(get_value_from_map(request_.key().c_str()));
                StartWrite(&response_);
            }

            void OnWriteDone(bool ok) override
            {
                if (!ok)
                {
                    return Finish(grpc::Status::CANCELLED);
                }
                StartRead(&request_);
            }

            void OnDone() override { delete this; }

           private:
            Request request_;
            Response response_;
        };

        return new Reactor();
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    KeyValueStoreServiceImpl service;

    ServerBuilder builder;
    // 在给定的地址上监听，不使用任何认证机制。
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 注册 "service" 作为与客户端通信的实例。
    // 在本例中，它对应一个 *同步* 服务。
    builder.RegisterService(&service);
    std::vector<std::unique_ptr<ServerInterceptorFactoryInterface>> creators;
    creators.push_back(std::unique_ptr<ServerInterceptorFactoryInterface>(
        new LoggingInterceptorFactory()));
    builder.experimental().SetInterceptorCreators(std::move(creators));
    // 最后组装服务器。
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器关闭。请注意，必须有其他线程负责关闭服务器，
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
