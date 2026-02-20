#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "hello_world.grpc.pb.h"
#include "helper.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(int, requests, 1000, "Number of requests");
ABSL_FLAG(int, concurrency, 1, "Number of concurrent threads");
ABSL_FLAG(bool, streaming, false, "Use streaming RPC");

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

    std::string SayHello(const std::string& user)
    {
        HelloRequest request;
        request.set_name(user);

        HelloReply reply;
        ClientContext context;

        Status status = stub_->SayHello(&context, request, &reply);

        if (status.ok())
        {
            return reply.message();
        }
        else
        {
            std::cerr << status.error_code() << ": " << status.error_message()
                      << std::endl;
            return "RPC failed";
        }
    }

   private:
    std::unique_ptr<Greeter::Stub> stub_;
};

void worker(GreeterClient* client, int num_requests,
            std::atomic<long>* total_time, std::atomic<int>* success_count)
{
    for (int i = 0; i < num_requests; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::string response = client->SayHello("world");

        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (response.find("Hello world") != std::string::npos)
        {
            success_count->fetch_add(1, std::memory_order_relaxed);
            total_time->fetch_add(duration.count(), std::memory_order_relaxed);
        }

        // 添加小的延迟以避免过载
        if (i % 100 == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

constexpr char kRootCertificate[] = "../../credentials/root.crt";

int main(int argc, char** argv)
{
    absl::ParseCommandLine(argc, argv);
    std::string target_str = absl::GetFlag(FLAGS_target);
    int total_requests = absl::GetFlag(FLAGS_requests);
    int concurrency = absl::GetFlag(FLAGS_concurrency);

    // 为通道构建 SSL 选项
    grpc::SslCredentialsOptions ssl_options;
    ssl_options.pem_root_certs = LoadStringFromFile(kRootCertificate);
    auto channel_creds = grpc::SslCredentials(ssl_options);

    std::cout << "=== SSL Client Performance Test ===" << std::endl;
    std::cout << "Target: " << target_str << std::endl;
    std::cout << "Total requests: " << total_requests << std::endl;
    std::cout << "Concurrency: " << concurrency << std::endl;

    // 为每个线程创建客户端
    std::vector<std::unique_ptr<GreeterClient>> clients;
    std::vector<std::thread> threads;

    for (int i = 0; i < concurrency; ++i)
    {
        auto channel = grpc::CreateChannel(target_str, channel_creds);
        clients.push_back(std::make_unique<GreeterClient>(channel));
    }

    std::atomic<long> total_time_us(0);
    std::atomic<int> success_count(0);

    auto start_test = std::chrono::high_resolution_clock::now();

    // 启动工作线程
    int requests_per_thread = total_requests / concurrency;
    for (int i = 0; i < concurrency; ++i)
    {
        threads.emplace_back(worker, clients[i].get(), requests_per_thread,
                             &total_time_us, &success_count);
    }

    // 等待所有线程完成
    for (auto& t : threads)
    {
        t.join();
    }

    auto end_test = std::chrono::high_resolution_clock::now();
    auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_test - start_test);

    // 计算结果
    double avg_latency_us =
        success_count > 0 ? static_cast<double>(total_time_us) / success_count
                          : 0;
    double qps = test_duration.count() > 0
                     ? (success_count * 1000.0) / test_duration.count()
                     : 0;

    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Total time: " << test_duration.count() << " ms" << std::endl;
    std::cout << "Successful requests: " << success_count << "/"
              << total_requests << std::endl;
    std::cout << "Average latency: " << avg_latency_us << " µs" << std::endl;
    std::cout << "QPS: " << qps << " requests/second" << std::endl;

    return 0;
}