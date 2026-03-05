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
 * WITHOUT WARRANTIES OR CONDITIONS CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Description: KV storage engine for etcd-lite
 */
#ifndef ETCDLITE_KV_STORE_H
#define ETCDLITE_KV_STORE_H

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "etcd/api/mvccpb/kv.pb.h"
#include "etcd/api/etcdserverpb/rpc.pb.h"
#include "status.h"

namespace etcdlite {

class KVStore {
public:
    KVStore() = default;
    ~KVStore() = default;

    /**
     * @brief Put a key-value pair
     * @param key The key
     * @param value The value
     * @param lease The lease ID (0 for no lease)
     * @param prevKv Output for previous KV if prev_kv is true
     * @return Status of the operation
     */
    Status Put(const std::string& key, const std::string& value, int64_t lease,
              mvccpb::KeyValue* prevKv = nullptr);

    /**
     * @brief Get a key-value pair
     * @param key The key
     * @param kv Output KeyValue
     * @return Status of the operation
     */
    Status Get(const std::string& key, mvccpb::KeyValue* kv);

    /**
     * @brief Range query
     * @param start Start key
     * @param end End key (empty for single key, "\0" for all keys >= start)
     * @param kvs Output key-value pairs
     * @param limit Maximum number of results (0 for no limit)
     * @param countOnly Return only count
     * @param keysOnly Return only keys
     * @return Status of the operation
     */
    Status Range(const std::string& start, const std::string& end,
                std::vector<mvccpb::KeyValue>* kvs, int64_t limit = 0,
                bool countOnly = false, bool keysOnly = false);

    /**
     * @brief Delete a key or range of keys
     * @param key Start key
     * @param rangeEnd End key (empty for single key)
     * @param prevKvs Output for previous KVs if prev_kv is true
     * @return Status of the operation
     */
    Status Delete(const std::string& key, const std::string& rangeEnd,
                std::vector<mvccpb::KeyValue>* prevKvs = nullptr);

    /**
     * @brief Get current revision
     */
    int64_t CurrentRevision() const { return revision_.load(); }

    /**
     * @brief Get cluster ID
     */
    uint64_t GetClusterId() const { return clusterId_.load(); }

    /**
     * @brief Get member ID
     */
    uint64_t GetMemberId() const { return memberId_.load(); }

    /**
     * @brief Set cluster ID
     */
    void SetClusterId(uint64_t clusterId) { clusterId_.store(clusterId); }

    /**
     * @brief Set member ID
     */
    void SetMemberId(uint64_t memberId) { memberId_.store(memberId); }

    /**
     * @brief Execute a transaction
     * @param compares Compare operations
     * @param success Success operations
     * @param failure Failure operations
     * @param response Output response
     * @return Status of the operation
     */
    Status Txn(const std::vector<etcdserverpb::Compare>& compares,
              const std::vector<etcdserverpb::RequestOp>& success,
              const std::vector<etcdserverpb::RequestOp>& failure,
              etcdserverpb::TxnResponse* response);

    /**
     * @brief Set watch callbacks
'     */
    void SetWatchCallback(
        std::function<void(const std::string&, const mvccpb::KeyValue&, const mvccpb::KeyValue*)> putCb,
        std::function<void(const std::string&, const mvccpb::KeyValue&)> delCb);

    /**
     * @brief Check if a key exists
     */
    bool KeyExists(const std::string& key) const;

private:
    struct KeyInfo {
        mvccpb::KeyValue kv;
        int64_t createRevision;
        int64_t modRevision;
        int64_t version;
    };

    /**
     * @brief Evaluate a compare operation
     */
    bool EvaluateCompare(const etcdserverpb::Compare& cmp, const KeyInfo* info) const;

    /**
     * @brief Check if a key is in range
     */
    bool KeyInRange(const std::string& key, const std::string& start, const std::string& end) const;

    std::unordered_map<std::string, KeyInfo> data_;
    std::atomic<int64_t> revision_{1};
    std::atomic<uint64_t> clusterId_{1};
    std::atomic<uint64_t> memberId_{1};
    mutable std::mutex mutex_;

    std::function<void(const std::string&, const mvccpb::KeyValue&, const mvccpb::KeyValue*)> putCallback_;
    std::function<void(const std::string&, const mvccpb::KeyValue&)> deleteCallback_;
};

}  // namespace etcdlite

#endif  // ETCDLITE_KV_STORE_H
