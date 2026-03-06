/**
 * WatchManagerTest.cpp - Tests for WatchManager class
 *
 * This file contains unit tests for the WatchManager class in etcd-lite.
 */

#include "watch/watch_manager.h"
#include "gtest/gtest.h"
#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "etcd/api/mvccpb/kv.pb.h"

namespace etcdlite {

class WatchManagerTest : public ::testing::Test {
protected:
    WatchManager manager;

    void SetUp() override {
        // Setup code here
    }

    void TearDown() override {
        // Teardown code here
    }
};

TEST_F(WatchManagerTest, RegisterWatcher) {
    etcdserverpb::WatchCreateRequest req;
    req.set_key("test_key");
    req.set_prev_kv(true);

    int64_t watchId = 0;

    // 使用 nullptr 作为 stream，因为我们只测试 WatchManager 的核心功能
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);
}

TEST_F(WatchManagerTest, CancelWatcher) {
    etcdserverpb::WatchCreateRequest req;
    req.set_key("test_key");
    req.set_prev_kv(true);

    int64_t watchId = 0;
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);

    status = manager.CancelWatcher(watchId);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(WatchManagerTest, NotifyPut) {
    // Create a watcher
    etcdserverpb::WatchCreateRequest req;
    req.set_key("test_key");
    req.set_prev_kv(true);

    int64_t watchId = 0;
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);

    // Notify about a put
    mvccpb::KeyValue kv;
    kv.set_key("test_key");
    kv.set_value("test_value");
    manager.NotifyPut("test_key", kv, nullptr, 1);

    // Cleanup
    status = manager.CancelWatcher(watchId);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(WatchManagerTest, NotifyDelete) {
    // Create a watcher
    etcdserverpb::WatchCreateRequest req;
    req.set_key("test_key");
    req.set_prev_kv(true);

    int64_t watchId = 0;
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);

    // Notify about a delete
    mvccpb::KeyValue kv;
    kv.set_key("test_key");
    kv.set_value("test_value");
    manager.NotifyDelete("test_key", kv, 1);

    // Cleanup
    status = manager.CancelWatcher(watchId);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(WatchManagerTest, RangeEndWatch) {
    // Create a watcher for a range
    etcdserverpb::WatchCreateRequest req;
    req.set_key("key_");
    req.set_range_end("key_z");
    req.set_prev_kv(true);

    int64_t watchId = 0;
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);

    // Notify about a put in range
    mvccpb::KeyValue kv;
    kv.set_key("key_b");
    kv.set_value("test_value");
    manager.NotifyPut("key_b", kv, nullptr, 1);

    // Cleanup
    status = manager.CancelWatcher(watchId);
    EXPECT_TRUE(status.IsOk());
}

TEST_F(WatchManagerTest, SetClusterAndMemberId) {
    manager.SetClusterId(12345);
    manager.SetMemberId(67890);

    // Verify by creating a watcher and checking
    etcdserverpb::WatchCreateRequest req;
    req.set_key("test_key");

    int64_t watchId = 0;
    Status status = manager.RegisterWatcher(req, nullptr, &watchId, nullptr);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(watchId, 0);

    status = manager.CancelWatcher(watchId);
    EXPECT_TRUE(status.IsOk());
}

} // namespace etcdlite
