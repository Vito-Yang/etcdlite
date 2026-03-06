/**
 * Test file for MaintenanceServiceImpl - maintenance service implementation
 */

#include <gtest/gtest.h>
#include "services/maintenance_service_impl.h"
#include "kv/kv_store.h"

namespace etcdlite {

class MaintenanceServiceTest : public ::testing::Test {
protected:
    KVStore kv_store_;
    MaintenanceServiceImpl service_;

    MaintenanceServiceTest() : service_(&kv_store_) {}

    void SetUp() override {
        // 测试前的初始化代码
    }

    void TearDown() override {
        // 测试后的清理代码
    }
};

// 测试 MaintenanceServiceImpl - Status API
TEST_F(MaintenanceServiceTest, Status) {
    etcdserverpb::StatusRequest req;
    etcdserverpb::StatusResponse resp;
    ::grpc::ServerContext context;

    // 调用 Status 方法
    ::grpc::Status grpcStatus = service_.Status(&context, &req, &resp);
    EXPECT_TRUE(grpcStatus.ok());

    // 验证返回的状态信息
    EXPECT_EQ(resp.version(), "3.5.0");
    EXPECT_GE(resp.dbsize(), 0);
    EXPECT_EQ(resp.leader(), 1);  // 默认自己是 leader
    EXPECT_EQ(resp.raftterm(), 1);

    // 验证 header 信息
    EXPECT_EQ(resp.header().cluster_id(), kv_store_.GetClusterId());
    EXPECT_EQ(resp.header().member_id(), kv_store_.GetMemberId());
    EXPECT_EQ(resp.header().revision(), kv_store_.CurrentRevision());
    EXPECT_EQ(resp.header().raft_term(), 1);

    // 验证 raft 信息
    EXPECT_EQ(resp.raftindex(), kv_store_.CurrentRevision());
    EXPECT_EQ(resp.raftterm(), 1);

    // 验证其他字段
    EXPECT_EQ(resp.dbsize(), 0);
    EXPECT_EQ(resp.raftappliedindex(), 0);
    EXPECT_EQ(resp.dbsizeinuse(), 0);
    EXPECT_FALSE(resp.islearner());
}

// 测试 MaintenanceServiceImpl - 不同 Cluster/Member ID
TEST_F(MaintenanceServiceTest, StatusWithCustomIds) {
    // 测试自定义的 clusterId 和 memberId
    kv_store_.SetClusterId(12345);
    kv_store_.SetMemberId(67890);

    etcdserverpb::StatusRequest req;
    etcdserverpb::StatusResponse resp;
    ::grpc::ServerContext context;

    ::grpc::Status grpcStatus = service_.Status(&context, &req, &resp);
    EXPECT_TRUE(grpcStatus.ok());

    EXPECT_EQ(resp.header().cluster_id(), 12345);
    EXPECT_EQ(resp.header().member_id(), 67890);
}

// 测试 MaintenanceServiceImpl - 随着操作 revision 的变化
TEST_F(MaintenanceServiceTest, StatusWithRevisionChange) {
    int64_t initialRev = kv_store_.CurrentRevision();
    etcdserverpb::StatusResponse resp;

    {
        etcdserverpb::StatusRequest req;
        ::grpc::ServerContext context;
        ::grpc::Status grpcStatus = service_.Status(&context, &req, &resp);
        EXPECT_TRUE(grpcStatus.ok());
        EXPECT_EQ(resp.header().revision(), initialRev);
    }

    // 执行一些操作以增加 revision
    kv_store_.Put("testkey", "testvalue", 0);
    EXPECT_GT(kv_store_.CurrentRevision(), initialRev);

    {
        etcdserverpb::StatusRequest req;
        ::grpc::ServerContext context;
        ::grpc::Status grpcStatus = service_.Status(&context, &req, &resp);
        EXPECT_TRUE(grpcStatus.ok());
        EXPECT_EQ(resp.header().revision(), kv_store_.CurrentRevision());
        EXPECT_EQ(resp.raftindex(), kv_store_.CurrentRevision());
    }
}

// 测试 MaintenanceServiceImpl - Raft 状态
TEST_F(MaintenanceServiceTest, StatusRaftState) {
    etcdserverpb::StatusRequest req;
    etcdserverpb::StatusResponse resp;
    ::grpc::ServerContext context;

    ::grpc::Status grpcStatus = service_.Status(&context, &req, &resp);
    EXPECT_TRUE(grpcStatus.ok());

    EXPECT_EQ(resp.raftterm(), 1);
    EXPECT_EQ(resp.leader(), 1);
    EXPECT_EQ(resp.raftindex(), kv_store_.CurrentRevision());
    EXPECT_EQ(resp.raftterm(), 1);
    EXPECT_EQ(resp.raftappliedindex(), 0);
    EXPECT_FALSE(resp.islearner());
}

}  // namespace etcdlite
