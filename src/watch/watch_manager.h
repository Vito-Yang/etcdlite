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
 * Description: Watch manager for etcd-lite
 */
#ifndef ETCDLITE_WATCH_MANAGER_H
#define ETCDLITE_WATCH_MANAGER_H

#include <atomic>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "etcd/api/mvccpb/kv.pb.h"
#include "kv/kv_store.h"
#include "status.h"

namespace etcdlite {

class WatchManager {
public:
    struct Watcher {
        int64_t watchId;
        std::string key;
        std::string rangeEnd;
        int64_t startRevision;
        bool prevKv;
        // Note: stream is a raw pointer because its lifetime is managed by gRPC framework
        grpc::ServerReaderWriter<etcdserverpb::WatchResponse, etcdserverpb::WatchRequest>* stream;
        std::atomic<bool> active{true};
        std::mutex streamMutex_;
        uint64_t clusterId;
        uint64_t memberId;
    };

    WatchManager() = default;
    ~WatchManager();

    /**
     * @brief Register a watcher
     * @param req The watch create request
     * @param stream The gRPC stream (raw pointer, lifetime managed by gRPC)
     * @param watchId Output watch ID
     * @param kvStore The KV store for historical events
     * @return Status of operation
     */
    Status RegisterWatcher(const etcdserverpb::WatchCreateRequest& req,
                         grpc::ServerReaderWriter<etcdserverpb::WatchResponse, etcdserverpb::WatchRequest>* stream,
                         int64_t* watchId, KVStore* kvStore);

    /**
     * @brief Cancel a watcher
     * @param watchId The watch ID
     * @return Status of operation
     */
    Status CancelWatcher(int64_t watchId);

    /**
     * @brief Notify watchers of a put event
     * @param key The key
     * @param kv The key-value pair
     * @param prevKv Previous key-value (optional)
     * @param revision Current revision
     */
    void NotifyPut(const std::string& key, const mvccpb::KeyValue& kv,
                  const mvccpb::KeyValue* prevKv = nullptr, int64_t revision = 0);

    /**
     * @brief Notify watchers of a delete event
     * @param key The key
     * @param kv The key-value pair (the deleted one)
     * @param revision Current revision
     */
    void NotifyDelete(const std::string& key, const mvccpb::KeyValue& kv, int64_t revision = 0);

    /**
     * @brief Set cluster ID
     */
    void SetClusterId(uint64_t clusterId) { clusterId_ = clusterId; }

    /**
     * @brief Set member ID
     */
    void SetMemberId(uint64_t memberId) { memberId_ = memberId; }

private:
    /**
     * @brief Check if a key matches a watcher's range
     */
    bool KeyMatchesWatcher(const std::string& key, const Watcher* watcher) const;

    /**
     * @brief Send a watch response
     */
    bool SendWatchResponse(Watcher* watcher, etcdserverpb::WatchResponse* response);

    /**
     * @brief Send historical events for a watcher
     */
    void SendHistoricalEvents(Watcher* watcher, KVStore* kvStore);

    std::unordered_map<int64_t, std::shared_ptr<Watcher>> watchers_;
    std::atomic<int64_t> nextWatchId_{1};
    std::atomic<uint64_t> clusterId_{1};
    std::atomic<uint64_t> memberId_{1};
    std::mutex mutex_;
};

}  // namespace etcdlite

#endif  // ETCDLITE_WATCH_MANAGER_H
