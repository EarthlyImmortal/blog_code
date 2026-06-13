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

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "caching_interceptor.h"
#include "key_value_store.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using keyvaluestore::KeyValueStore;
using keyvaluestore::Request;
using keyvaluestore::Response;

class KeyValueStoreClient
{
   public:
    explicit KeyValueStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KeyValueStore::NewStub(channel))
    {
    }

    // 发送一组 key，接收对应的 value 并打印
    void GetValues(const std::vector<std::string>& keys)
    {
        class GetValuesReactor
            : public grpc::ClientBidiReactor<Request, Response>
        {
           public:
            GetValuesReactor(KeyValueStore::Stub* stub,
                             const std::vector<std::string>& keys)
                : keys_(keys)
            {
                stub->async()->GetValues(&context_, this);
                assert(!keys_.empty());
                request_.set_key(keys_[0]);
                StartWrite(&request_);
                StartCall();
            }

            void OnReadDone(bool ok) override
            {
                if (ok)
                {
                    std::cout << request_.key() << " : " << response_.value()
                              << std::endl;
                    if (++counter_ < keys_.size())
                    {
                        request_.set_key(keys_[counter_]);
                        StartWrite(&request_);
                    }
                    else
                    {
                        StartWritesDone();
                    }
                }
            }

            void OnWriteDone(bool ok) override
            {
                if (ok)
                {
                    StartRead(&response_);
                }
            }

            void OnDone(const grpc::Status& status) override
            {
                std::lock_guard<std::mutex> lock(mu_);
                status_ = status;
                done_ = true;
                cv_.notify_all();
            }

            Status Await()
            {
                std::unique_lock<std::mutex> lock(mu_);
                cv_.wait(lock, [this] { return done_; });
                return status_;
            }

           private:
            ClientContext context_;
            const std::vector<std::string> keys_;
            size_t counter_ = 0;
            Request request_;
            Response response_;
            std::mutex mu_;
            std::condition_variable cv_;
            Status status_;
            bool done_ = false;
        };

        GetValuesReactor reactor(stub_.get(), keys);
        Status status = reactor.Await();
        if (!status.ok())
        {
            std::cout << "RPC failed: " << status.error_code() << ": "
                      << status.error_message() << std::endl;
        }
    }

   private:
    std::unique_ptr<KeyValueStore::Stub> stub_;
};

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();

    grpc::ChannelArguments args;
    std::vector<
        std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
        interceptor_creators;
    interceptor_creators.push_back(
        std::make_unique<CachingInterceptorFactory>());
    auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
        "localhost:50051", grpc::InsecureChannelCredentials(), args,
        std::move(interceptor_creators));

    KeyValueStoreClient client(channel);
    std::vector<std::string> keys = {"key1", "key2", "key3", "key4",
                                     "key5", "key1", "key2", "key4"};
    client.GetValues(keys);
    client.GetValues(keys);
    return 0;
}