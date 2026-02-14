# 一元RPC(Unary RPC)
```mermaid
sequenceDiagram
    participant Client as 客户端
    participant Framework as gRPC框架
    participant Reactor as Reactor对象

    Client->>Framework: 发送Unary RPC请求
    Note right of Client: 包含Point数据
    
    Framework->>+Reactor: 调用GetFeature()创建Reactor
    Note right of Framework: 传入point、feature等参数
    
    Reactor->>Reactor: 构造函数执行
    Note right of Reactor: 1. 设置feature名称<br/>2. 设置location<br/>3. 调用Finish(grpc::Status::OK)
    
    Reactor->>Framework: 返回Reactor指针
    deactivate Reactor
    
    Reactor->>Framework: Finish()信号
    Note right of Reactor: 非阻塞调用<br/>通知框架可以发送响应
    
    Framework->>Client: 异步发送响应
    Note left of Framework: 通过HTTP/2流发送响应数据
    
    Framework->>Reactor: 触发OnDone()
    Note right of Reactor: 整个RPC完全结束时调用
    
    Reactor->>Reactor: OnDone()执行
    Note right of Reactor: 记录"RPC Completed"<br/>执行delete this
    
    opt 客户端取消请求
        Client->>Framework: 发送取消请求
        Framework->>Reactor: 触发OnCancel()
        Reactor->>Reactor: OnCancel()执行
        Note right of Reactor: 记录"RPC Cancelled"
    end
```

# 服务器端流式 RPC(Server-side streaming RPC)
```mermaid
sequenceDiagram
    participant Client as 客户端
    participant Framework as gRPC框架
    participant Reactor as Lister Reactor对象

    Note over Client, Reactor: 1. RPC建立与Reactor初始化
    Client->>+Framework: 发送ListFeatures请求
    Note right of Client: 包含Rectangle数据
    Framework->>+Reactor: 创建Lister(rectangle, feature_list)
    Note right of Framework: 调用ListFeatures方法
    Reactor->>Reactor: 构造函数执行
    Note right of Reactor: 1. 计算区域边界<br/>2. 初始化迭代器<br/>3. 调用NextWrite()
    Reactor->>-Framework: 返回Reactor指针

    Note over Client, Reactor: 2. 流式数据发送循环
    loop 对于每个符合条件的特征
        Reactor->>Framework: StartWrite(&feature)
        Note right of Reactor: 异步写入单个特征
        Framework->>Client: 通过HTTP/2流发送特征数据
        Framework->>Reactor: OnWriteDone(ok)
        Reactor->>Reactor: 检查ok，调用NextWrite()
    end

    Note over Client, Reactor: 3. 流结束与资源清理
    Reactor->>Framework: Finish(Status::OK)
    Framework->>Client: 发送流结束信号
    Framework->>Reactor: OnDone()
    Reactor->>Reactor: 记录日志并delete this

    Note over Client, Reactor: 4. 异常路径：客户端取消请求
    opt 客户端取消请求
        Client->>Framework: 发送取消请求
        Framework->>Reactor: OnCancel()
        Reactor->>Reactor: 记录"RPC Cancelled"
        # 框架可能会终止连接，OnDone可能不会被调用
    end
```

# 客户端流式传输RPC(Client-side streaming RPC)
```mermaid
sequenceDiagram
    participant Client as 客户端
    participant Framework as gRPC框架
    participant Reactor as Recorder Reactor对象

    Note over Client, Reactor: 1. RPC建立与Reactor初始化
    Client->>+Framework: 调用RecordRoute RPC
    Framework->>+Reactor: 创建Recorder(summary, feature_list)
    Note right of Reactor: 调用RecordRoute方法
    Reactor->>Reactor: 构造函数执行
    Note right of Reactor: 1. 记录开始时间<br/>2. 初始化计数器<br/>3. 调用StartRead(&point_)
    Reactor->>-Framework: 返回Reactor指针

    Note over Client, Reactor: 2. 流式数据读取循环
    loop 对于每个客户端发送的点
        Client->>Framework: 流式发送Point数据
        Framework->>Reactor: OnReadDone(ok=true)
        Reactor->>Reactor: 处理点数据
        Note right of Reactor: 1. 增加点计数<br/>2. 检查特征计数<br/>3. 计算距离<br/>4. 更新前一个点
        Reactor->>Framework: StartRead(&point_) 准备下一读取
    end

    Note over Client, Reactor: 3. 流结束与资源清理
    Client->>Framework: 发送流结束信号
    Framework->>Reactor: OnReadDone(ok=false)
    Reactor->>Reactor: 设置RouteSummary摘要
    Note right of Reactor: 1. 设置点数量<br/>2. 设置特征数量<br/>3. 设置总距离<br/>4. 计算耗时
    Reactor->>Framework: Finish(Status::OK)
    Framework->>Client: 返回RouteSummary响应
    Framework->>Reactor: OnDone()
    Reactor->>Reactor: 记录日志并delete this

    Note over Client, Reactor: 4. 异常路径：客户端取消请求
    opt 客户端取消请求
        Client->>Framework: 发送取消请求
        Framework->>Reactor: OnCancel()
        Reactor->>Reactor: 记录"RecordRoute RPC Cancelled"
        # 框架可能会终止连接，OnDone可能不会被调用
    end
```
# 双向流式 RPC(Bidirectional streaming RPC)
```mermaid
sequenceDiagram
    participant Client as 客户端
    participant Framework as gRPC框架
    participant Reactor as Chatter Reactor对象

    Note over Client, Reactor: 1. RPC连接建立与Reactor初始化
    Client->>+Framework: 发起RouteChat RPC调用
    Framework->>+Reactor: 创建Chatter(mu, received_notes)
    Note right of Framework: 调用RouteChat方法
    Reactor->>Reactor: 构造函数执行
    Note right of Reactor: 1. 初始化成员变量<br/>2. 调用StartRead(&note_)
    Reactor->>-Framework: 返回Reactor指针

    Note over Client, Reactor: 2. 双向流式通信循环
    loop 持续通信直到流结束
        Note over Client, Reactor: 2.1 客户端发送消息
        Client->>Framework: 发送RouteNote消息
        Framework->>Reactor: OnReadDone(true)
        Reactor->>Reactor: 处理接收到的note_
        Note right of Reactor: 1. 加锁筛选相关注释<br/>2. 初始化发送迭代器<br/>3. 调用NextWrite()

        Note over Client, Reactor: 2.2 服务端回复相关消息
        loop 对于每个相关注释
            Reactor->>Framework: StartWrite(&*notes_iterator_)
            Framework->>Client: 发送RouteNote响应
            Framework->>Reactor: OnWriteDone(true)
            Reactor->>Reactor: NextWrite()继续发送
        end

        Reactor->>Reactor: 保存当前note_并开始新读取
        Note right of Reactor: 1. 加锁保存note_到received_notes_<br/>2. 调用StartRead(&note_)
    end

    Note over Client, Reactor: 3. 正常结束与资源清理
    alt 读取失败（流结束）
        Framework->>Reactor: OnReadDone(false)
        Reactor->>Framework: Finish(Status::OK)
        Framework->>Client: 发送流结束信号
        Framework->>Reactor: OnDone()
        Reactor->>Reactor: 记录日志并delete this
    end

    Note over Client, Reactor: 4. 异常路径：客户端取消请求
    opt 客户端取消请求
        Client->>Framework: 发送取消请求
        Framework->>Reactor: OnCancel()
        Reactor->>Reactor: 记录"RouteChat RPC Cancelled"
    end
```