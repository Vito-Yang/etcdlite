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
 * Description: KV storage engine implementation
 */
#include "kv/kv_store.h"

#include <algorithm>

namespace etcdlite {

Status KVStore::Put(const std::string& key, const std::string& value, int64_t lease,
                    mvccpb::KeyValue* prevKv) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t newRevision = ++revision_;

    // 保存之前的KV
    bool existed = data_.find(key) != data_.end();
    mvccpb::KeyValue prevKvCopy;
    if (existed) {
        prevKvCopy = data_[key].kv;
    }

    // 创建新的KV
    KeyInfo info;
    info.kv.set_key(key);
    info.kv.set_value(value);
    info.kv.set_lease(lease);
    info.kv.set_create_revision(existed ? data_[key].createRevision : newRevision);
    info.kv.set_mod_revision(newRevision);
    info.kv.set_version(existed ? data_[key].version + 1 : 1);
    info.createRevision = info.kv.create_revision();
    info.modRevision = info.kv.mod_revision();
    info.version = info.kv.version();

    data_[key] = info;

    // 触发Watch事件
    if (putCallback_) {
        if (existed) {
            putCallback_(key, info.kv, &prevKvCopy);
        } else {
            putCallback_(key, info.kv, nullptr);
        }
    }

    // 返回之前的KV
    if (prevKv && existed) {
        *prevKv = prevKvCopy;
    }

    return Status::OK();
}

Status KVStore::Get(const std::string& key, mvccpb::KeyValue* kv) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) {
        return Status(StatusCode::NOT_FOUND, "Key not found");
    }
    *kv = it->second.kv;
    return Status::OK();
}

bool KVStore::KeyInRange(const std::string& key, const std::string& start,
                         const std::string& end) const {
    if (key < start) {
        return false;
    }
    if (end.empty()) {
        return key == start;  // 单key
    }
    if (end[0] == 0) {
        return key >= start;  // 范围查询 [start, ...)
    }
    return key < end;  // [start, end)
}

Status KVStore::Range(const std::string& start, const std::string& end,
                     std::vector<mvccpb::KeyValue>* kvs, int64_t limit,
                     bool countOnly, bool keysOnly) {
    std::lock_guard<std::mutex> lock(mutex_);

    int64_t count = 0;
    for (const auto& [key, info] : data_) {
        if (!KeyInRange(key, start, end)) {
            continue;
        }
        count++;
        if (limit > 0 && count > limit) {
            break;
        }
        if (!countOnly) {
            if (keysOnly) {
                mvccpb::KeyValue kv;
                kv.set_key(key);
                kvs->push_back(kv);
            } else {
                kvs->push_back(info.kv);
            }
        }
    }

    return Status::OK();
}

Status KVStore::Delete(const std::string& key, const std::string& rangeEnd,
                      std::vector<mvccpb::KeyValue>* prevKvs) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t newRevision = ++revision_;

    std::vector<std::string> keysToDelete;
    std::vector<mvccpb::KeyValue> deletedKvs;

    // 找出要删除的keys
    for (auto it = data_.begin(); it != data_.end(); ++it) {
        if (KeyInRange(it->first, key, rangeEnd)) {
            keysToDelete.push_back(it->first);
            if (prevKvs) {
                deletedKvs.push_back(it->second.kv);
            }
        }
    }

    // 删除keys
    for (const auto& k : keysToDelete) {
        data_.erase(k);
    }

    // 触发Watch事件
    if (deleteCallback_) {
        for (const auto& kv : deletedKvs) {
            deleteCallback_(kv.key(), kv);
        }
    }

    // 返回之前的KVs
    if (prevKvs) {
        *prevKvs = deletedKvs;
    }

    return Status::OK();
}

bool KVStore::KeyExists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.find(key) != data_.end();
}

bool KVStore::EvaluateCompare(const etcdserverpb::Compare& cmp, const KeyInfo* info) const {
    if (!info) {
        return false;
    }

    auto getTargetValue = [&]() -> int64_t {
        switch (cmp.target()) {
            case etcdserverpb::Compare::VERSION:
                return info->version;
            case etcdserverpb::Compare::CREATE:
                return info->createRevision;
            case etcdserverpb::Compare::MOD:
                return info->modRevision;
            case etcdserverpb::Compare::LEASE:
                return info->kv.lease();
            default:
                return 0;
        }
    };

    switch (cmp.target()) {
        case etcdserverpb::Compare::VALUE: {
            const auto& kvValue = info->kv.value();
            const auto& cmpValue = cmp.value();
            switch (cmp.result()) {
                case etcdserverpb::Compare::EQUAL:
                    return kvValue == cmpValue;
                case etcdserverpb::Compare::NOT_EQUAL:
                    return kvValue != cmpValue;
                case etcdserverpb::Compare::GREATER:
                    return kvValue > cmpValue;
                case etcdserverpb::Compare::LESS:
                    return kvValue < cmpValue;
            }
            break;
        }
        case etcdserverpb::Compare::VERSION:
        case etcdserverpb::Compare::CREATE:
        case etcdserverpb::Compare::MOD:
        case etcdserverpb::Compare::LEASE: {
            int64_t targetValue = getTargetValue();
            switch (cmp.result()) {
                case etcdserverpb::Compare::EQUAL:
                    return targetValue == cmp.version();
                case etcdserverpb::Compare::NOT_EQUAL:
                    return targetValue != cmp.version();
                case etcdserverpb::Compare::GREATER:
                    return targetValue > cmp.version();
                case etcdserverpb::Compare::LESS:
                    return targetValue < cmp.version();
            }
            break;
        }
    }
    return false;
}

Status KVStore::Txn(const std::vector<etcdserverpb::Compare>& compares,
                      const std::vector<etcdserverpb::RequestOp>& success,
                      const std::vector<etcdserverpb::RequestOp>& failure,
                      etcdserverpb::TxnResponse* response) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t newRevision = ++revision_;

    // 评估所有Compare
    bool compareResult = true;
    for (const auto& cmp : compares) {
        auto it = data_.find(cmp.key());
        if (!EvaluateCompare(cmp, it != data_.end() ? &it->second : nullptr)) {
            compareResult = false;
            break;
        }
    }

    // 执行成功或失败操作
    const auto& ops = compareResult ? success : failure;
    response->set_succeeded(compareResult);

    for (const auto& op : ops) {
        auto* respOp = response->add_responses();
        if (op.has_request_put()) {
            const auto& putReq = op.request_put();
            mvccpb::KeyValue prevKv;
            bool existed = data_.find(putReq.key()) != data_.end();
            if (existed) {
                prevKv = data_[putReq.key()].kv;
            }

            KeyInfo info;
            info.kv.set_key(putReq.key());
            info.kv.set_value(putReq.value());
            info.kv.set_lease(putReq.lease());
            info.kv.set_create_revision(existed ? data_[putReq.key()].createRevision : newRevision);
            info.kv.set_mod_revision(newRevision);
            info.kv.set_version(existed ? data_[putReq.key()].version + 1 : 1);
            info.createRevision = info.kv.create_revision();
            info.modRevision = info.kv.mod_revision();
            info.version = info.kv.version();

            data_[putReq.key()] = info;

            auto* putResp = respOp->mutable_response_put();
            if (putReq.prev_kv()) {
                *putResp->mutable_prev_kv() = prevKv;
            }

        } else if (op.has_request_delete_range()) {
            const auto& delReq = op.request_delete_range();

            std::vector<std::string> keysToDelete;
            std::vector<mvccpb::KeyValue> deletedKvs;

            for (auto it = data_.begin(); it != data_.end(); ++it) {
                if (KeyInRange(it->first, delReq.key(), delReq.range_end())) {
                    keysToDelete.push_back(it->first);
                    if (delReq.prev_kv()) {
                        deletedKvs.push_back(it->second.kv);
                    }
                }
            }

            for (const auto& k : keysToDelete) {
                data_.erase(k);
            }

            auto* delResp = respOp->mutable_response_delete_range();
            delResp->set_deleted(keysToDelete.size());
            if (delReq.prev_kv()) {
                for (const auto& kv : deletedKvs) {
                    *delResp->add_prev_kvs() = kv;
                }
            }
        } else if (op.has_request_range()) {
            const auto& rangeReq = op.request_range();
            std::vector<mvccpb::KeyValue> kvs;
            Range(rangeReq.key(), rangeReq.range_end(), &kvs, rangeReq.limit(),
                  rangeReq.count_only(), rangeReq.keys_only());

            auto* rangeResp = respOp->mutable_response_range();
            for (const auto& kv : kvs) {
                *rangeResp->add_kvs() = kv;
            }
            rangeResp->set_count(kvs.size());
            rangeResp->set_more(false);
        }
    }

    // 填充response header
    auto* header = response->mutable_header();
    header->set_cluster_id(clusterId_.load());
    header->set_member_id(memberId_.load());
    header->set_revision(newRevision);
    header->set_raft_term(1);

    return Status::OK();
}

void KVStore::SetWatchCallback(
    std::function<void(const std::string&, const mvccpb::KeyValue&, const mvccpb::KeyValue*)> putCb,
    std::function<void(const std::string&, const mvccpb::KeyValue&)> delCb) {
    putCallback_ = putCb;
    deleteCallback_ = delCb;
}

}  // namespace etcdlite
