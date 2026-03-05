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
 * Description: Watch manager implementation
 */
#include "watch/watch_manager.h"

namespace etcdlite {

WatchManager::~WatchManager()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, watcher] : watchers_) {
        watcher->active.store(false);
    }
}

bool WatchManager::KeyMatchesWatcher(const std::string& key, const Watcher* watcher) const
{
    if (key < watcher->key) {
        return false;
    }
    if (watcher->rangeEnd.empty()) {
        return key == watcher->key;  // 单key
    }
    if (watcher->rangeEnd[0] == 0) {
        return key >= watcher->key;  // 范围查询 [key, ...)
    }
    return key < watcher->rangeEnd;  // [key, rangeEnd)
}

Status WatchManager::RegisterWatcher(const etcdserverpb::WatchCreateRequest& req,
                                  grpc::ServerReaderWriter<etcdserverpb::WatchResponse, etcdserverpb::WatchRequest>* stream,
                                  int64_t* watchId, KVStore* kvStore)
{
    auto watcher = std::make_shared<Watcher>();
    *watchId = nextWatchId_++;

    watcher->watchId = *watchId;
    watcher->key = req.key();
    watcher->rangeEnd = req.range_end();
    watcher->startRevision = req.start_revision();
    watcher->prevKv = req.prev_kv();
    watcher->stream = stream;
    watcher->clusterId = clusterId_.load();
    watcher->memberId = memberId_.load();

    // 发送created响应
    etcdserverpb::WatchResponse resp;
    auto* header = resp.mutable_header();
    header->set_cluster_id(watcher->clusterId);
    header->set_member_id(watcher->memberId);
    header->set_revision(kvStore->CurrentRevision());
    resp.set_watch_id(*watchId);
    resp.set_created(true);

    if (!SendWatchResponse(watcher.get(), &resp)) {
        return Status(StatusCode::INTERNAL, "Failed to send watch created response");
    }

    // 发送历史事件（如果指定了startRevision）
    if (req.start_revision() > 0) {
        SendHistoricalEvents(watcher.get(), kvStore);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    watchers_[*watchId] = watcher;

    return Status::OK();
}

Status WatchManager::CancelWatcher(int64_t watchId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = watchers_.find(watchId);
    if (it == watchers_.end()) {
        return Status(StatusCode::NOT_FOUND, "Watcher not found");
    }

    // 发送canceled响应
    etcdserverpb::WatchResponse resp;
    auto* header = resp.mutable_header();
    header->set_cluster_id(clusterId_.load());
    header->set_member_id(memberId_.load());
    resp.set_watch_id(watchId);
    resp.set_canceled(true);

    SendWatchResponse(it->second.get(), &resp);
    it->second->active.store(false);
    watchers_.erase(it);

    return Status::OK();
}

void WatchManager::NotifyPut(const std::string& key, const mvccpb::KeyValue& kv,
                            const mvccpb::KeyValue* prevKv, int64_t revision)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [id, watcher] : watchers_) {
        if (!watcher->active.load()) {
            continue;
        }
        if (!KeyMatchesWatcher(key, watcher.get())) {
            continue;
        }

        etcdserverpb::WatchResponse resp;
        auto* header = resp.mutable_header();
        header->set_cluster_id(watcher->clusterId);
        header->set_member_id(watcher->memberId);
        header->set_revision(revision);
        resp.set_watch_id(watcher->watchId);

        auto* event = resp.add_events();
        event->set_type(mvccpb::Event::PUT);
        *event->mutable_kv() = kv;
        if (watcher->prevKv && prevKv) {
            *event->mutable_prev_kv() = *prevKv;
        }

        SendWatchResponse(watcher.get(), &resp);
    }
}

void WatchManager::NotifyDelete(const std::string& key, const mvccpb::KeyValue& kv, int64_t revision)
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [id, watcher] : watchers_) {
        if (!watcher->active.load()) {
            continue;
        }
        if (!KeyMatchesWatcher(key, watcher.get())) {
            continue;
        }

        etcdserverpb::WatchResponse resp;
        auto* header = resp.mutable_header();
        header->set_cluster_id(watcher->clusterId);
        header->set_member_id(watcher->memberId);
        header->set_revision(revision);
        resp.set_watch_id(watcher->watchId);

        auto* event = resp.add_events();
        event->set_type(mvccpb::Event::DELETE);
        *event->mutable_kv() = kv;

        SendWatchResponse(watcher.get(), &resp);
    }
}

bool WatchManager::SendWatchResponse(Watcher* watcher, etcdserverpb::WatchResponse* response)
{
    std::lock_guard<std::mutex> lock(watcher->streamMutex_);
    return watcher->stream->Write(*response);
}

void WatchManager::SendHistoricalEvents(Watcher* watcher, KVStore* kvStore)
{
    // 发送指定revision之后的所有历史事件
    // 简化实现：发送当前所有匹配的keys作为PUT事件
    // 真实的etcd应该发送revision >= startRevision的所有历史事件

    std::vector<mvccpb::KeyValue> kvs;
    Status status = kvStore->Range(watcher->key, watcher->rangeEnd, &kvs);
    if (status.IsError()) {
        return;
    }

    for (const auto& kv : kvs) {
        etcdserverpb::WatchResponse resp;
        auto* header = resp.mutable_header();
        header->set_cluster_id(watcher->clusterId);
        header->set_member_id(watcher->memberId);
        header->set_revision(kv.mod_revision());
        resp.set_watch_id(watcher->watchId);

        auto* event = resp.add_events();
        event->set_type(mvccpb::Event::PUT);
        *event->mutable_kv() = kv;

        SendWatchResponse(watcher, &resp);
    }
}

}  // namespace etcdlite
