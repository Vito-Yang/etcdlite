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
 * Description: Lease manager implementation
 */
#include "lease/lease_manager.h"

#include <algorithm>

namespace etcdlite {

LeaseManager::~LeaseManager()
{
    StopExpirationCheck();
}

Status LeaseManager::Grant(int64_t ttl, int64_t* leaseId, int64_t* actualTtl)
{
    std::lock_guard<std::mutex> lock(mutex_);
    *leaseId = nextLeaseId_++;
    *actualTtl = ttl;

    LeaseInfo info;
    info.leaseId = *leaseId;
    info.ttl = ttl;
    info.expireTime = std::chrono::steady_clock::now() + std::chrono::seconds(ttl);
    leases_[*leaseId] = info;

    return Status::OK();
}

Status LeaseManager::Revoke(int64_t leaseId)
{
    std::vector<std::string> keys;
    Status rc = GetLeaseKeys(leaseId, &keys);
    if (rc.IsError()) {
        return rc;
    }

    // 删除关联的keys
    if (deleteKeyCallback_) {
        for (const auto& key : keys) {
            deleteKeyCallback_(key);
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    leases_.erase(leaseId);
    return Status::OK();
}

Status LeaseManager::KeepAlive(int64_t leaseId, int64_t* ttl)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return Status(StatusCode::NOT_FOUND, "Lease not found");
    }

    // 更新过期时间
    it->second.expireTime = std::chrono::steady_clock::now() + std::chrono::seconds(it->second.ttl);
    *ttl = it->second.ttl;

    return Status::OK();
}

Status LeaseManager::TimeToLive(int64_t leaseId, int64_t* ttl, std::vector<std::string>* keys)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return Status(StatusCode::NOT_FOUND, "Lease not found");
    }

    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        it->second.expireTime - now).count();
    *ttl = remaining > 0 ? remaining : -1;

    if (keys) {
        keys->insert(keys->end(), it->second.keys.begin(), it->second.keys.end());
    }

    return Status::OK();
}

Status LeaseManager::AttachKey(int64_t leaseId, const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return Status(StatusCode::NOT_FOUND, "Lease not found");
    }
    it->second.keys.insert(key);
    return Status::OK();
}

Status LeaseManager::DetachKey(int64_t leaseId, const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return Status(StatusCode::NOT_FOUND, "Lease not found");
    }
    it->second.keys.erase(key);
    return Status::OK();
}

Status LeaseManager::GetLeaseKeys(int64_t leaseId, std::vector<std::string>* keys)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return Status(StatusCode::NOT_FOUND, "Lease not found");
    }
    keys->insert(keys->end(), it->second.keys.begin(), it->second.keys.end());
    return Status::OK();
}

bool LeaseManager::LeaseExists(int64_t leaseId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return leases_.find(leaseId) != leases_.end();
}

int64_t LeaseManager::GetLeaseTTL(int64_t leaseId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = leases_.find(leaseId);
    if (it == leases_.end()) {
        return -1;
    }
    auto now = std::chrono::steady_clock::now();
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        it->second.expireTime - now).count();
    return remaining > 0 ? remaining : -1;
}

void LeaseManager::StartExpirationCheck()
{
    if (running_.load()) {
        return;
    }
    running_.store(true);
    expirationThread_ = std::thread([this]() {
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            CheckExpiration();
        }
    });
}

void LeaseManager::StopExpirationCheck()
{
    running_.store(false);
    if (expirationThread_.joinable()) {
        expirationThread_.join();
    }
}

void LeaseManager::CheckExpiration()
{
    auto now = std::chrono::steady_clock::now();
    std::vector<int64_t> expiredLeases;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [leaseId, info] : leases_) {
            if (now >= info.expireTime) {
                expiredLeases.push_back(leaseId);
            }
        }
    }

    // 撤销过期的租约
    for (auto leaseId : expiredLeases) {
        Revoke(leaseId);
    }
}

}  // namespace etcdlite
