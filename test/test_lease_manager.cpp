/**
 * LeaseManagerTest.cpp - Tests for LeaseManager class
 *
 * This file contains unit tests for the LeaseManager class in etcd-lite.
 */

#include "lease/lease_manager.h"
#include "gtest/gtest.h"
#include <thread>
#include <chrono>

namespace etcdlite {

class LeaseManagerTest : public ::testing::Test {
protected:
    LeaseManager manager;

    void SetUp() override {
        // Setup code here
    }

    void TearDown() override {
        // Teardown code here
    }
};

TEST_F(LeaseManagerTest, GrantLease) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);
    EXPECT_GT(actualTtl, 0);
    EXPECT_LE(actualTtl, 60);
}

TEST_F(LeaseManagerTest, RevokeLease) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    status = manager.Revoke(leaseId);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(LeaseManagerTest, KeepAlive) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    int64_t ttl = 0;
    status = manager.KeepAlive(leaseId, &ttl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(ttl, 0);
}

TEST_F(LeaseManagerTest, TimeToLive) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    int64_t ttl = 0;
    std::vector<std::string> keys;
    status = manager.TimeToLive(leaseId, &ttl, &keys);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(ttl, 0);
    EXPECT_EQ(keys.size(), 0);
}

TEST_F(LeaseManagerTest, AttachAndDetachKey) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    status = manager.AttachKey(leaseId, "test_key");
    EXPECT_TRUE(status.IsOk());

    std::vector<std::string> keys;
    int64_t ttl = 0;
    status = manager.TimeToLive(leaseId, &ttl, &keys);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "test_key");

    status = manager.DetachKey(leaseId, "test_key");
    EXPECT_TRUE(status.IsOk());

    keys.clear();
    status = manager.TimeToLive(leaseId, &ttl, &keys);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(keys.size(), 0);
}

TEST_F(LeaseManagerTest, LeaseExists) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    EXPECT_TRUE(manager.LeaseExists(leaseId));
    EXPECT_FALSE(manager.LeaseExists(99999));
}

TEST_F(LeaseManagerTest, GetLeaseKeys) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    status = manager.AttachKey(leaseId, "key1");
    EXPECT_TRUE(status.IsOk());
    status = manager.AttachKey(leaseId, "key2");
    EXPECT_TRUE(status.IsOk());

    std::vector<std::string> keys;
    status = manager.GetLeaseKeys(leaseId, &keys);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(keys.size(), 2);
}

TEST_F(LeaseManagerTest, GetLeaseTTL) {
    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = manager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(leaseId, 0);

    int64_t ttl = manager.GetLeaseTTL(leaseId);
    EXPECT_GT(ttl, 0);
    EXPECT_LE(ttl, 60);

    EXPECT_EQ(manager.GetLeaseTTL(99999), -1);
}

// 测试 LeaseManager - 过期检查线程启动和停止
TEST_F(LeaseManagerTest, ExpirationCheckThread) {
    // 测试启动和停止过期检查线程
    manager.StartExpirationCheck();
    manager.StopExpirationCheck();
    EXPECT_TRUE(true);  // 只要不崩溃就通过
}

// 测试 LeaseManager - 过期检查和删除键回调
TEST_F(LeaseManagerTest, ExpirationCallback) {
    // 设置删除键回调
    std::vector<std::string> deletedKeys;
    manager.SetDeleteKeyCallback(
        [&deletedKeys](const std::string& key) {
            deletedKeys.push_back(key);
            return Status::OK();
        }
    );

    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    manager.Grant(1, &leaseId, &actualTtl);  // TTL 1 秒
    manager.AttachKey(leaseId, "expire_key");

    // 启动过期检查
    manager.StartExpirationCheck();

    // 等待 lease 过期
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 停止过期检查
    manager.StopExpirationCheck();

    // 验证键被删除
    EXPECT_FALSE(manager.LeaseExists(leaseId));
    EXPECT_FALSE(deletedKeys.empty());
    EXPECT_EQ(deletedKeys[0], "expire_key");
}

} // namespace etcdlite
