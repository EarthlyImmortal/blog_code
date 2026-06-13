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

#include <grpcpp/support/client_interceptor.h>

#include <map>

#include "absl/log/check.h"
#include "key_value_store.grpc.pb.h"

// 这是一个缓存的朴素实现。每次调用都会创建一个新的缓存。
// 对于每个新的键请求，首先在 map 中搜索该键，如果找到，
// 拦截器会填充返回值，而不向服务器发送请求。
// 只有在缓存中未找到键时，我们才发出请求。
class CachingInterceptor : public grpc::experimental::Interceptor
{
   public:
    CachingInterceptor(grpc::experimental::ClientRpcInfo* info)
    {
        std::ignore = info;
        std::cout << "CachingInterceptor created!" << std::endl;
    }

    void Intercept(
        ::grpc::experimental::InterceptorBatchMethods* methods) override
    {
        bool hijack = false;
        if (methods->QueryInterceptionHookPoint(
                grpc::experimental::InterceptionHookPoints::
                    PRE_SEND_INITIAL_METADATA))
        {
            // 劫持所有调用
            hijack = true;
            // 创建一个流，此拦截器可以通过该流发出请求
            stub_ = keyvaluestore::KeyValueStore::NewStub(
                methods->GetInterceptedChannel());
            stream_ = stub_->GetValues(&context_);
        }
        if (methods->QueryInterceptionHookPoint(
                grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE))
        {
            // 我们知道客户端在循环中执行 Read 和 Write，因此
            // 不需要维护响应列表。
            std::string requested_key;
            const keyvaluestore::Request* req_msg =
                static_cast<const keyvaluestore::Request*>(
                    methods->GetSendMessage());
            if (req_msg != nullptr)
            {
                requested_key = req_msg->key();
            }
            else
            {
                // 在某些场景下，非序列化形式可能不可用，因此添加一个回退方案
                keyvaluestore::Request req_msg;
                auto* buffer = methods->GetSerializedSendMessage();
                auto copied_buffer = *buffer;
                CHECK(grpc::SerializationTraits<
                          keyvaluestore::Request>::Deserialize(&copied_buffer,
                                                               &req_msg)
                          .ok());
                requested_key = req_msg.key();
            }

            // 检查键是否存在于 map 中
            auto search = cached_map_.find(requested_key);
            if (search != cached_map_.end())
            {
                std::cout << requested_key << " found in map" << std::endl;
                response_ = search->second;
            }
            else
            {
                std::cout << requested_key << " not found in cache"
                          << std::endl;
                // 在缓存中未找到键，因此发出请求
                keyvaluestore::Request req;
                req.set_key(requested_key);
                stream_->Write(req);
                keyvaluestore::Response resp;
                stream_->Read(&resp);
                response_ = resp.value();
                // 将键值对插入缓存以供将来请求使用
                cached_map_.insert({requested_key, response_});
            }
        }
        if (methods->QueryInterceptionHookPoint(
                grpc::experimental::InterceptionHookPoints::PRE_SEND_CLOSE))
        {
            stream_->WritesDone();
        }
        if (methods->QueryInterceptionHookPoint(
                grpc::experimental::InterceptionHookPoints::PRE_RECV_MESSAGE))
        {
            keyvaluestore::Response* resp =
                static_cast<keyvaluestore::Response*>(
                    methods->GetRecvMessage());
            resp->set_value(response_);
        }
        if (methods->QueryInterceptionHookPoint(
                grpc::experimental::InterceptionHookPoints::PRE_RECV_STATUS))
        {
            auto* status = methods->GetRecvStatus();
            *status = grpc::Status::OK;
        }
        // 必须始终调用 Hijack 或 Proceed 之一以推进处理。
        if (hijack)
        {
            // 仅在挂钩点包含 PRE_SEND_INITIAL_METADATA 时调用一次 Hijack
            methods->Hijack();
        }
        else
        {
            // Proceed 表示该拦截器已完成对这批操作的拦截。
            methods->Proceed();
        }
    }

    ~CachingInterceptor()
    {
        std::cout << "CachingInterceptor destoryed!" << std::endl;
    }

   private:
    grpc::ClientContext context_;
    std::unique_ptr<keyvaluestore::KeyValueStore::Stub> stub_;
    std::unique_ptr<grpc::ClientReaderWriter<keyvaluestore::Request,
                                             keyvaluestore::Response>>
        stream_;
    std::map<std::string, std::string> cached_map_;
    std::string response_;
};

class CachingInterceptorFactory
    : public grpc::experimental::ClientInterceptorFactoryInterface
{
   public:
    grpc::experimental::Interceptor* CreateClientInterceptor(
        grpc::experimental::ClientRpcInfo* info) override
    {
        return new CachingInterceptor(info);
    }
};
