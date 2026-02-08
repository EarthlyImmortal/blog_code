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

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "helper.h"
#ifdef BAZEL_BUILD
#include "examples/protos/route_guide.grpc.pb.h"
#else
#include "route_guide.grpc.pb.h"
#endif

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;
using routeguide::Feature;
using routeguide::Point;
using routeguide::Rectangle;
using routeguide::RouteGuide;
using routeguide::RouteNote;
using routeguide::RouteSummary;
using std::chrono::system_clock;

// 将数值转换为弧度
float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

// 该公式基于 http://mathforum.org/library/drmath/view/51879.html
// 计算两点之间的距离
float GetDistance(const Point& start, const Point& end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

  float a = pow(sin(delta_lat_rad / 2), 2) +
            cos(lat_rad_1) * cos(lat_rad_2) * pow(sin(delta_lon_rad / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  int R = 6371000;  // 单位为米

  return R * c;
}

// 根据给定的点，从特征列表中查找对应的特征名称
std::string GetFeatureName(const Point& point,
                           const std::vector<Feature>& feature_list) {
  for (const Feature& f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

// RouteGuide 服务的实现类，使用回调服务 API
class RouteGuideImpl final : public RouteGuide::CallbackService {
 public:
  explicit RouteGuideImpl(const std::string& db) {
    routeguide::ParseDb(db, &feature_list_);
  }

  // 简单 RPC 方法实现：获取给定点的特征
  grpc::ServerUnaryReactor* GetFeature(grpc::CallbackServerContext* context,
                                       const Point* point,
                                       Feature* feature) override {
    // 内部反应器类，处理请求和响应
    class Reactor : public grpc::ServerUnaryReactor {
     public:
      Reactor(const Point& point, const std::vector<Feature>& feature_list,
              Feature* feature) {
        feature->set_name(GetFeatureName(point, feature_list));
        *feature->mutable_location() = point;
        Finish(grpc::Status::OK); // 完成 RPC 并返回状态
      }

     private:
      // RPC 完成时调用
      void OnDone() override {
        LOG(INFO) << "GetFeature RPC Completed";
        delete this;
      }

      // RPC 被取消时调用
      void OnCancel() override { LOG(ERROR) << "GetFeature RPC Cancelled"; }
    };
    return new Reactor(*point, feature_list_, feature);
  }

  /* 使用 DefaultReactor 实现 GetFeature 的另一种简单方法
  grpc::ServerUnaryReactor* GetFeature(CallbackServerContext* context,
                                       const Point* point,
                                       Feature* feature) override {
    feature->set_name(GetFeatureName(*point, feature_list_));
    feature->mutable_location()->CopyFrom(*point);
    auto* reactor = context->DefaultReactor();
    reactor->Finish(Status::OK);
    return reactor;
  }
*/

  // 服务器端流式 RPC 方法实现：列出给定矩形区域内的特征
  grpc::ServerWriteReactor<Feature>* ListFeatures(
      CallbackServerContext* context,
      const routeguide::Rectangle* rectangle) override {
    // 内部反应器类，用于流式写入多个特征
    class Lister : public grpc::ServerWriteReactor<Feature> {
     public:
      Lister(const routeguide::Rectangle* rectangle,
             const std::vector<Feature>* feature_list)
          : left_((std::min)(rectangle->lo().longitude(),
                             rectangle->hi().longitude())),
            right_((std::max)(rectangle->lo().longitude(),
                              rectangle->hi().longitude())),
            top_((std::max)(rectangle->lo().latitude(),
                            rectangle->hi().latitude())),
            bottom_((std::min)(rectangle->lo().latitude(),
                               rectangle->hi().latitude())),
            feature_list_(feature_list),
            next_feature_(feature_list_->begin()) {
        NextWrite(); // 开始写入过程
      }

      // 单个写入操作完成时调用
      void OnWriteDone(bool ok) override {
        if (!ok) {
          Finish(Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
          return;
        }
        NextWrite(); // 继续下一个写入
      }

      void OnDone() override {
        LOG(INFO) << "ListFeatures RPC Completed";
        delete this;
      }

      void OnCancel() override { LOG(ERROR) << "ListFeatures RPC Cancelled"; }

     private:
      // 准备并发送下一个可用的特征
      void NextWrite() {
        while (next_feature_ != feature_list_->end()) {
          const Feature& f = *next_feature_;
          next_feature_++;
          if (f.location().longitude() >= left_ &&
              f.location().longitude() <= right_ &&
              f.location().latitude() >= bottom_ &&
              f.location().latitude() <= top_) {
            StartWrite(&f); // 将特征写入流
            return;
          }
        }
        // 没有可写内容，全部完成
        Finish(Status::OK);
      }
      const long left_;
      const long right_;
      const long top_;
      const long bottom_;
      const std::vector<Feature>* feature_list_;
      std::vector<Feature>::const_iterator next_feature_;
    };
    return new Lister(rectangle, &feature_list_);
  }

  // 客户端流式 RPC 方法实现：记录路径并返回摘要
  grpc::ServerReadReactor<Point>* RecordRoute(CallbackServerContext* context,
                                              RouteSummary* summary) override {
    // 内部反应器类，用于流式读取点
    class Recorder : public grpc::ServerReadReactor<Point> {
     public:
      Recorder(RouteSummary* summary, const std::vector<Feature>* feature_list)
          : start_time_(system_clock::now()),
            summary_(summary),
            feature_list_(feature_list) {
        StartRead(&point_); // 开始读取客户端发送的点
      }

      // 单个读取操作完成时调用
      void OnReadDone(bool ok) override {
        if (ok) {
          point_count_++;
          if (!GetFeatureName(point_, *feature_list_).empty()) {
            feature_count_++;
          }
          if (point_count_ != 1) {
            distance_ += GetDistance(previous_, point_);
          }
          previous_ = point_;
          StartRead(&point_); // 继续读取下一个点
        } else {
          // 客户端流结束，设置摘要并完成 RPC
          summary_->set_point_count(point_count_);
          summary_->set_feature_count(feature_count_);
          summary_->set_distance(static_cast<long>(distance_));
          auto secs = std::chrono::duration_cast<std::chrono::seconds>(
              system_clock::now() - start_time_);
          summary_->set_elapsed_time(secs.count());
          Finish(Status::OK);
        }
      }

      void OnDone() override {
        LOG(INFO) << "RecordRoute RPC Completed";
        delete this;
      }

      void OnCancel() override { LOG(ERROR) << "RecordRoute RPC Cancelled"; }

     private:
      system_clock::time_point start_time_;
      RouteSummary* summary_;
      const std::vector<Feature>* feature_list_;
      Point point_;
      int point_count_ = 0;
      int feature_count_ = 0;
      float distance_ = 0.0;
      Point previous_;
    };
    return new Recorder(summary, &feature_list_);
  }

  // 双向流式 RPC 方法实现：路由聊天，双方独立发送和接收路由注释
  grpc::ServerBidiReactor<RouteNote, RouteNote>* RouteChat(
      CallbackServerContext* context) override {
    // 内部反应器类，用于双向流式通信
    class Chatter : public grpc::ServerBidiReactor<RouteNote, RouteNote> {
     public:
      Chatter(absl::Mutex* mu, std::vector<RouteNote>* received_notes)
          : mu_(mu), received_notes_(received_notes) {
        StartRead(&note_); // 开始读取客户端发送的注释
      }

      // 读取操作完成时调用
      void OnReadDone(bool ok) override {
        if (ok) {
          // 与此目录中未使用反应器模式的其他示例不同，
          // 我们无法获取本地锁来保护对注释向量的访问，
          // 因为反应器很可能会使我们跳转到不同线程，
          // 所以我们必须使用不同的锁定策略。
          // 我们将在本地获取锁以构建要发送的注释列表副本，
          // 然后再次获取锁以将接收到的注释追加到现有向量。
          mu_->Lock();
          std::copy_if(received_notes_->begin(), received_notes_->end(),
                       std::back_inserter(to_send_notes_),
                       [this](const RouteNote& note) {
                         return note.location().latitude() ==
                                    note_.location().latitude() &&
                                note.location().longitude() ==
                                    note_.location().longitude();
                       });
          mu_->Unlock();
          notes_iterator_ = to_send_notes_.begin();
          NextWrite(); // 开始向客户端发送相关注释
        } else {
          Finish(Status::OK);
        }
      }
      void OnWriteDone(bool /*ok*/) override { NextWrite(); }

      void OnDone() override {
        LOG(INFO) << "RouteChat RPC Completed";
        delete this;
      }

      void OnCancel() override { LOG(ERROR) << "RouteChat RPC Cancelled"; }

     private:
      // 准备并发送下一个注释
      void NextWrite() {
        if (notes_iterator_ != to_send_notes_.end()) {
          StartWrite(&*notes_iterator_);
          notes_iterator_++;
        } else {
          // 所有相关注释已发送，将接收到的注释保存并继续读取
          mu_->Lock();
          received_notes_->push_back(note_);
          mu_->Unlock();
          StartRead(&note_);
        }
      }

      RouteNote note_;
      absl::Mutex* mu_;
      std::vector<RouteNote>* received_notes_ ABSL_GUARDED_BY(mu_);
      std::vector<RouteNote> to_send_notes_;
      std::vector<RouteNote>::iterator notes_iterator_;
    };
    return new Chatter(&mu_, &received_notes_);
  }

 private:
  std::vector<Feature> feature_list_;
  absl::Mutex mu_;
  std::vector<RouteNote> received_notes_ ABSL_GUARDED_BY(mu_);
};

// 运行 gRPC 服务器
void RunServer(const std::string& db_path) {
  std::string server_address("0.0.0.0:50051");
  RouteGuideImpl service(db_path);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG(INFO) << "Server listening on " << server_address;
  server->Wait(); // 阻塞等待服务器结束
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::InitializeLog();
  // 预期只有一个参数：--db_path=path/to/route_guide_db.json
  std::string db = routeguide::GetDbFileContent(argc, argv);
  RunServer(db);

  return 0;
}
