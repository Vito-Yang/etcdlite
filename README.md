# EtcdLite - 轻量级 etcd v3 gRPC Server 实现

## 简介

EtcdLite 是一个轻量级的 etcd v3 gRPC Server 实现，用于模拟 etcd server 的核心功能。该项目独立编译，可以单独被 yuanrong-datasystem 项目或其他 etcd SDK 客户端使用。

### 设计目标

- 消除 yuanrong-datasystem 项目对真实 etcd server 的依赖
- 提供兼容 etcd v3 gRPC API 的内存 KV 存储
- 支持项目实际使用的 etcd 功能

## 功能特性

### 已实现的 etcd v3 API

| 服务 | 方法 | 说明 |
|------|------|------|
| **KV Service** | `Put` | 写入 KV，支持 prev_kv |
| | `Range` | 读取 KV，支持范围查询、前缀查询、count_only、keys_only |
| | `DeleteRange` | 删除 KV，支持范围删除、前缀删除、prev_kv |
| | `Txn` | 事务操作，支持 Compare + Put/DeleteRange |
| **Watch Service** | `Watch` (流式) | 监听 key 变化，支持创建/取消 watcher |
| **Lease Service** | `LeaseGrant` | 创建租约 |
| | `LeaseKeepAlive` (流式) | 租约续期（流式） |
| | `LeaseRevoke` | 撤销租约（内部使用） |
| **Maintenance Service** | `Status` | 返回集群状态信息 |

### 未实现的服务

- ~~Cluster Service~~ - 客户端未使用
- ~~Auth Service~~ - 客户端未使用
- ~~Election Service~~ - 客户端通过 Lease + KV 模拟选举

## 项目结构

```
etcdlite/
├── CMakeLists.txt                    # CMake 构建配置
├── README.md                         # 项目文档（本文件）
├── src/
│   ├── etcdlite.h                  # 公共 API 头文件
│   ├── server.h                   # EtcdLiteServer 主类
│   ├── server.cpp
│   ├── main.cpp                     # 命令行入口
│   ├── status.h                    # 状态类
│   ├── status.cpp
│   ├── kv/
│   │   ├── kv_store.h           # KV 存储引擎
│   │   └── kv_store.cpp
│   ├── lease/
│   │   ├── lease_manager.h      # 租约管理
│   │   └── lease_manager.cpp
│   ├── watch/
│   │   ├── watch_manager.h      # 监听管理
│   │   └── watch_manager.cpp
│   └── services/                # gRPC 服务实现
│       ├── kv_service_impl.h
│       ├── kv_service_impl.cpp
│       ├── lease_service_impl.h
│       ├── lease_service_impl.cpp
│       ├── watch_service_impl.h
│       ├── watch_service_impl.cpp
│       └── maintenance_service_impl.h
│       └── maintenance_service_impl.cpp
└── third_party/
    └── protos/                    # etcd proto 文件
        └── etcd/api/etcdserverpb/
        │       └── rpc.proto
        └── etcd/api/mvccpb/
        │       └── kv.proto
        └── ...
```

## 编译

### 依赖

- CMake >= 3.15
- C++17 支持
- gRPC (gRPC::grpc++, gRPC::grpc++_reflection)
- Protobuf (protobuf::libprotobuf)
- pthread

### 编译步骤

```bash
cd /home/hhc/yr_wp/etcdlite
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 编译产物

- `libetcdlite.so` - 共享库
- `etcdlite-server` - 可执行文件
- `libetcdlite_proto.so` - 生成的 proto 库

## 运行

### 基本运行

```bash
# 默认地址: 0.0.0.0:2379
./build/etcdlite-server

# 指定地址
./build/etcdlite-server 127.0.0.1:2379
```

### 信号处理

服务器支持 SIGINT 和 SIGTERM 信号，接收到后会优雅关闭。

### 输出示例

```
EtcdLite server started on 0.0.0.0:2379
```

## 集成到 yuanrong-datasystem

### 方式 1：独立运行（推荐）

启动 etcdlite server：
```bash
# 终端1
/home/hhc/yr_wp/etcdlite/build/etcdlite-server 0.0.0.0:2379
```

配置 yuanrong-datasystem 连接到 etcdlite：
```bash
# 终端2
./worker --etcd-endpoints=127.0.0.1:2379
```

### 方式 2：作为 CMake 依赖集成

在 yuanrong-datasystem 的 `CMakeLists.txt` 中添加：

```cmake
# 添加 etcdlite 依赖
set(ETCDLITE_DIR /home/hhc/yr_wp/etcdlite/build)
find_package(etcdlite PATHS ${ETCDLITE_DIR} REQUIRED)

# 链接到你的 target
target_link_libraries(your_target PRIVATE etcdlite::etcdlite)
target_include_directories(your_target PRIVATE ${ETCDLITE_DIR}/../src)
```

然后在代码中使用：

```cpp
#include "etcdlite.h"

etcdlite::EtcdLiteServer server;
server.Start("0.0.0.0:2379");
server.Wait();
```

## API 使用示例

### 作为共享库使用

```cpp
#include "etcdlite.h"

etcdlite::EtcdLiteServer server;

// 启动服务器
etcdlite::Status status = server.Start("0.0.0.0:2379");
if (!status.IsOk()) {
    std::cerr << "Failed to start: " << status.Message() << std::endl;
    return;
}

// 访问组件
auto* kvStore = server.GetKVStore();
auto* leaseManager = server.GetLeaseManager();
auto* watchManager = server.GetWatchManager();

// KV 操作
mvccpb::KeyValue kv;
status = kvStore->Put("key", "value", 0, nullptr);

// 等待服务器
server.Wait();
```

## 测试验证

### 1. 使用 etcdctl 测试

```bash
# 安装 etcdctl 或使用 go etcd client

# 写入
etcdctl put /test/key "hello"

# 读取
etcdctl get /test/key

# 删除
etcdctl del /test/key

# 监听
etcdctl watch /test/key &
```

### 2. 使用 yuanrong-datasystem 的 etcd_store 测试

连接到 `http://localhost:2379`，然后运行现有的 etcd_store 测试用例。

### 3. 健康检查

```bash
# etcd_health.cpp 会调用 Maintenance::Status
# 应该返回 200 OK
```

## 注意事项

1. **数据持久化**：EtcdLite 使用内存存储，进程重启后数据会丢失
2. **单实例模式**：设计为单实例运行，不支持分布式同步
3. **MVCC 支持**：简化实现，保存当前版本，不支持完整的历史版本查询
4. **租约过期**：后台线程每秒检查一次过期租约，过期的租约会自动删除关联的 key
5. **Watch 流式**：每个 Watch 连接使用独立线程处理
6. **Cluster ID**：默认使用固定值模拟单节点集群
7. **Proto 文件**：从 yuanrong-datasystem 的 `third_party/protos/` 目录复制

## 故障排查

### 编译问题

**问题**: `cmake: command not found`
**解决**: 确保 cmake 已安装并添加到 PATH，或使用完整路径

**问题**: 找不到 gRPC
**解决**: 确保 gRPC 已正确安装，使用 `find_package(gRPC CONFIG REQUIRED)`

**问题**: Proto 生成错误
**解决**: 检查 proto 文件是否正确复制，路径是否正确

### 运行时问题

**问题**: 绑定端口已被占用
**解决**: 使用其他端口或停止占用该端口的进程

**问题**: 客户端连接失败
**解决**: 检查网络配置，确保地址和端口正确

**问题**: Watch 事件未接收到
**解决**: 检查 Watch 注册时 key 格式是否正确

## 协议说明

本项目遵循 Apache License 2.0 协议。

## 联系

- **yuanrong-datasystem**: 使用本项目的项目
- **etcd v3**: [etcd 官方项目](https://github.com/etcd-io/etcd)
