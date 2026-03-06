/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Description: Unit tests for KVStore
 */

#include <gtest/gtest.h>
#include "kv/kv_store.h"
#include "lease/lease_manager.h"
#include "etcd/api/mvccpb/kv.pb.h"
#include "etcd/api/etcdserverpb/rpc.pb.h"

namespace etcdlite {

class KVStoreTest : public ::testing::Test {
protected:
    KVStore kv_store_;

    void SetUp() override {
        // 可以在此处添加测试前的初始化代码
    }

    void TearDown() override {
        // 可以在此处添加测试后的清理代码
    }
};

// 测试 Put 和 Get 操作
TEST_F(KVStoreTest, PutAndGet) {
    // 测试基本的 Put 和 Get
    Status status = kv_store_.Put("testkey", "testvalue", 0);
    EXPECT_TRUE(status.IsOk());

    mvccpb::KeyValue kv;
    status = kv_store_.Get("testkey", &kv);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kv.key(), "testkey");
    EXPECT_EQ(kv.value(), "testvalue");
}

// 测试 Put 操作返回前值
TEST_F(KVStoreTest, PutWithPrevKv) {
    Status status = kv_store_.Put("testkey", "oldvalue", 0);
    EXPECT_TRUE(status.IsOk());

    mvccpb::KeyValue prev_kv;
    status = kv_store_.Put("testkey", "newvalue", 0, &prev_kv);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(prev_kv.key(), "testkey");
    EXPECT_EQ(prev_kv.value(), "oldvalue");
}

// 测试 Delete 操作
TEST_F(KVStoreTest, Delete) {
    Status status = kv_store_.Put("testkey", "testvalue", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> prev_kvs;
    status = kv_store_.Delete("testkey", "", &prev_kvs);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(prev_kvs.size(), 1);
    EXPECT_EQ(prev_kvs[0].key(), "testkey");
    EXPECT_EQ(prev_kvs[0].value(), "testvalue");

    mvccpb::KeyValue kv;
    status = kv_store_.Get("testkey", &kv);
    EXPECT_FALSE(status.IsOk());
}

// 测试 KeyExists
TEST_F(KVStoreTest, KeyExists) {
    EXPECT_FALSE(kv_store_.KeyExists("nonexistent"));

    Status status = kv_store_.Put("testkey", "testvalue", 0);
    EXPECT_TRUE(status.IsOk());
    EXPECT_TRUE(kv_store_.KeyExists("testkey"));
}

// 测试 Range 操作 - 单个键
TEST_F(KVStoreTest, RangeSingleKey) {
    Status status = kv_store_.Put("testkey", "testvalue", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> kvs;
    status = kv_store_.Range("testkey", "", &kvs);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kvs.size(), 1);
    EXPECT_EQ(kvs[0].key(), "testkey");
    EXPECT_EQ(kvs[0].value(), "testvalue");
}

// 测试 Range 操作 - 范围查询
TEST_F(KVStoreTest, RangeQuery) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key3", "value3", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> kvs;
    status = kv_store_.Range("key1", "key3", &kvs);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kvs.size(), 2);  // key1 和 key2 在范围内
    EXPECT_EQ(kvs[0].key(), "key1");
    EXPECT_EQ(kvs[1].key(), "key2");
}

// 测试 Range 操作 - 前缀查询
TEST_F(KVStoreTest, RangePrefix) {
    Status status = kv_store_.Put("/test/key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("/test/key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("/other/key", "value3", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> kvs;
    status = kv_store_.Range("/test/", "/test0", &kvs);  // 前缀查询
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kvs.size(), 2);
    EXPECT_EQ(kvs[0].key(), "/test/key1");
    EXPECT_EQ(kvs[1].key(), "/test/key2");
}

// 测试 Range 操作 - 计数模式
TEST_F(KVStoreTest, RangeCountOnly) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key3", "value3", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> kvs;
    status = kv_store_.Range("key1", "key4", &kvs, 0, true);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kvs.size(), 3);
}

// 测试 Revision 递增
TEST_F(KVStoreTest, RevisionIncrement) {
    int64_t initial_rev = kv_store_.CurrentRevision();

    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(kv_store_.CurrentRevision(), initial_rev);

    int64_t rev_after_put = kv_store_.CurrentRevision();
    status = kv_store_.Put("key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(kv_store_.CurrentRevision(), rev_after_put);

    int64_t rev_after_put2 = kv_store_.CurrentRevision();
    status = kv_store_.Delete("key1", "");
    EXPECT_TRUE(status.IsOk());
    EXPECT_GT(kv_store_.CurrentRevision(), rev_after_put2);
}

// 测试 KeyInRange 内部方法
TEST_F(KVStoreTest, KeyInRange) {
    // 测试单个键范围
    EXPECT_TRUE(kv_store_.KeyInRange("testkey", "testkey", ""));
    EXPECT_FALSE(kv_store_.KeyInRange("otherkey", "testkey", ""));

    // 测试范围查询
    EXPECT_TRUE(kv_store_.KeyInRange("key2", "key1", "key3"));
    EXPECT_FALSE(kv_store_.KeyInRange("key0", "key1", "key3"));
    EXPECT_FALSE(kv_store_.KeyInRange("key4", "key1", "key3"));
}

// 测试 ClusterId 和 MemberId 管理
TEST_F(KVStoreTest, ClusterAndMemberId) {
    EXPECT_EQ(kv_store_.GetClusterId(), 1);
    EXPECT_EQ(kv_store_.GetMemberId(), 1);

    kv_store_.SetClusterId(12345);
    EXPECT_EQ(kv_store_.GetClusterId(), 12345);

    kv_store_.SetMemberId(67890);
    EXPECT_EQ(kv_store_.GetMemberId(), 67890);
}

// 测试 Range 操作 - 只返回键
TEST_F(KVStoreTest, RangeKeysOnly) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<mvccpb::KeyValue> kvs;
    status = kv_store_.Range("key1", "key3", &kvs, 0, false, true);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kvs.size(), 2);
    EXPECT_EQ(kvs[0].key(), "key1");
    EXPECT_EQ(kvs[0].value(), "");  // keysOnly 模式下 value 为空
    EXPECT_EQ(kvs[1].key(), "key2");
    EXPECT_EQ(kvs[1].value(), "");
}

// 测试 Txn 事务 - 简单成功场景
TEST_F(KVStoreTest, TxnSuccess) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<etcdserverpb::Compare> compares;
    etcdserverpb::Compare cmp;
    cmp.set_key("key1");
    cmp.set_target(etcdserverpb::Compare::VALUE);
    cmp.set_result(etcdserverpb::Compare::EQUAL);
    cmp.set_value("value1");
    compares.push_back(cmp);

    std::vector<etcdserverpb::RequestOp> success;
    etcdserverpb::RequestOp op;
    auto* put = op.mutable_request_put();
    put->set_key("key2");
    put->set_value("txn_value");
    success.push_back(op);

    std::vector<etcdserverpb::RequestOp> failure;

    etcdserverpb::TxnResponse response;
    status = kv_store_.Txn(compares, success, failure, &response);
    EXPECT_TRUE(status.IsOk());
    EXPECT_TRUE(response.succeeded());

    mvccpb::KeyValue kv;
    status = kv_store_.Get("key2", &kv);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kv.value(), "txn_value");
}

// 测试 Txn 事务 - 比较失败场景
TEST_F(KVStoreTest, TxnFailure) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<etcdserverpb::Compare> compares;
    etcdserverpb::Compare cmp;
    cmp.set_key("key1");
    cmp.set_target(etcdserverpb::Compare::VALUE);
    cmp.set_result(etcdserverpb::Compare::EQUAL);
    cmp.set_value("wrong_value");
    compares.push_back(cmp);

    std::vector<etcdserverpb::RequestOp> success;
    std::vector<etcdserverpb::RequestOp> failure;
    etcdserverpb::RequestOp op;
    auto* put = op.mutable_request_put();
    put->set_key("key2");
    put->set_value("failure_value");
    failure.push_back(op);

    etcdserverpb::TxnResponse response;
    status = kv_store_.Txn(compares, success, failure, &response);
    EXPECT_TRUE(status.IsOk());
    EXPECT_FALSE(response.succeeded());

    mvccpb::KeyValue kv;
    status = kv_store_.Get("key2", &kv);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kv.value(), "failure_value");
}

// 测试 Txn 事务 - 版本比较
TEST_F(KVStoreTest, TxnCompareVersion) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<etcdserverpb::Compare> compares;
    etcdserverpb::Compare cmp;
    cmp.set_key("key1");
    cmp.set_target(etcdserverpb::Compare::VERSION);
    cmp.set_result(etcdserverpb::Compare::EQUAL);
    cmp.set_version(1);
    compares.push_back(cmp);

    std::vector<etcdserverpb::RequestOp> success;
    std::vector<etcdserverpb::RequestOp> failure;

    etcdserverpb::TxnResponse response;
    status = kv_store_.Txn(compares, success, failure, &response);
    EXPECT_TRUE(status.IsOk());
    EXPECT_TRUE(response.succeeded());
}

// 测试 Txn 事务 - 删除操作
TEST_F(KVStoreTest, TxnDelete) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<etcdserverpb::Compare> compares;
    std::vector<etcdserverpb::RequestOp> success;
    etcdserverpb::RequestOp op;
    auto* del = op.mutable_request_delete_range();
    del->set_key("key1");
    del->set_prev_kv(true);
    success.push_back(op);

    std::vector<etcdserverpb::RequestOp> failure;

    etcdserverpb::TxnResponse response;
    status = kv_store_.Txn(compares, success, failure, &response);
    EXPECT_TRUE(status.IsOk());
    EXPECT_TRUE(response.succeeded());
    EXPECT_EQ(response.responses_size(), 1);
    EXPECT_EQ(response.responses(0).response_delete_range().deleted(), 1);
    EXPECT_EQ(response.responses(0).response_delete_range().prev_kvs_size(), 1);

    EXPECT_FALSE(kv_store_.KeyExists("key1"));
}

// 测试 Txn 事务 - 范围查询
TEST_F(KVStoreTest, TxnRange) {
    Status status = kv_store_.Put("key1", "value1", 0);
    EXPECT_TRUE(status.IsOk());
    status = kv_store_.Put("key2", "value2", 0);
    EXPECT_TRUE(status.IsOk());

    std::vector<etcdserverpb::Compare> compares;
    std::vector<etcdserverpb::RequestOp> success;
    etcdserverpb::RequestOp op;
    auto* range = op.mutable_request_range();
    range->set_key("key1");
    range->set_range_end("key3");
    success.push_back(op);

    std::vector<etcdserverpb::RequestOp> failure;

    etcdserverpb::TxnResponse response;
    status = kv_store_.Txn(compares, success, failure, &response);
    EXPECT_TRUE(status.IsOk());
    EXPECT_TRUE(response.succeeded());
    EXPECT_EQ(response.responses_size(), 1);
    EXPECT_EQ(response.responses(0).response_range().kvs_size(), 2);
    EXPECT_EQ(response.responses(0).response_range().count(), 2);
}

// 测试 KVStore 与 LeaseManager 集成 - Put with Lease
TEST_F(KVStoreTest, PutWithLease) {
    LeaseManager leaseManager;
    kv_store_.SetLeaseManager(&leaseManager);

    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = leaseManager.Grant(60, &leaseId, &actualTtl);
    EXPECT_TRUE(status.IsOk());

    // 用 lease 放一个键
    status = kv_store_.Put("lease_key", "lease_value", leaseId);
    EXPECT_TRUE(status.IsOk());

    // 验证键存在且有 lease
    mvccpb::KeyValue kv;
    status = kv_store_.Get("lease_key", &kv);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(kv.lease(), leaseId);

    // 验证 lease 关联了该键
    std::vector<std::string> keys;
    status = leaseManager.GetLeaseKeys(leaseId, &keys);
    EXPECT_TRUE(status.IsOk());
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "lease_key");
}

// 测试 KVStore 与 LeaseManager 集成 - 更新已有的 lease
TEST_F(KVStoreTest, UpdateWithDifferentLease) {
    LeaseManager leaseManager;
    kv_store_.SetLeaseManager(&leaseManager);

    int64_t leaseId1 = 0, leaseId2 = 0;
    int64_t actualTtl = 0;
    leaseManager.Grant(60, &leaseId1, &actualTtl);
    leaseManager.Grant(60, &leaseId2, &actualTtl);

    // 先用 lease1 放键
    kv_store_.Put("key", "value1", leaseId1);

    // 验证关联到 lease1
    std::vector<std::string> keys;
    leaseManager.GetLeaseKeys(leaseId1, &keys);
    EXPECT_EQ(keys.size(), 1);

    // 用 lease2 更新同一个键
    kv_store_.Put("key", "value2", leaseId2);

    // 验证已经从 lease1 解绑，绑定到 lease2
    keys.clear();
    leaseManager.GetLeaseKeys(leaseId1, &keys);
    EXPECT_EQ(keys.size(), 0);

    keys.clear();
    leaseManager.GetLeaseKeys(leaseId2, &keys);
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "key");
}

// 测试 KVStore 与 LeaseManager 集成 - 移除 lease
TEST_F(KVStoreTest, RemoveLeaseFromKey) {
    LeaseManager leaseManager;
    kv_store_.SetLeaseManager(&leaseManager);

    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    leaseManager.Grant(60, &leaseId, &actualTtl);

    // 放一个带 lease 的键
    kv_store_.Put("key", "value", leaseId);

    // 更新为不带 lease
    kv_store_.Put("key", "new_value", 0);

    // 验证键不再关联 lease
    std::vector<std::string> keys;
    leaseManager.GetLeaseKeys(leaseId, &keys);
    EXPECT_EQ(keys.size(), 0);

    mvccpb::KeyValue kv;
    kv_store_.Get("key", &kv);
    EXPECT_EQ(kv.lease(), 0);
}

// 测试 KVStore 与 LeaseManager 集成 - 删除带 lease 的键
TEST_F(KVStoreTest, DeleteKeyWithLease) {
    LeaseManager leaseManager;
    kv_store_.SetLeaseManager(&leaseManager);

    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    leaseManager.Grant(60, &leaseId, &actualTtl);

    kv_store_.Put("key", "value", leaseId);

    // 删除键
    kv_store_.Delete("key", "");

    // 验证键已经从 lease 解绑
    std::vector<std::string> keys;
    leaseManager.GetLeaseKeys(leaseId, &keys);
    EXPECT_EQ(keys.size(), 0);
}

}  // namespace etcdlite

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
