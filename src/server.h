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
 * Description: EtcdLiteServer - etcd gRPC server implementation
 */
#ifndef ETCDLITE_SERVER_H
#define ETCDLITE_SERVER_H

#include <memory>

#include <grpcpp/grpcpp.h>

#include "kv/kv_store.h"
#include "lease/lease_manager.h"
#include "watch/watch_manager.h"
#include "services/kv_service_impl.h"
#include "services/lease_service_impl.h"
#include "services/watch_service_impl.h"
#include "services/maintenance_service_impl.h"
#include "status.h"

namespace etcdlite {

class EtcdLiteServer {
public:
    EtcdLiteServer();
    ~EtcdLiteServer();

    /**
     * @brief Start gRPC server
     * @param address The server address (e.g., "0.0.0.0:2379")
     * @return Status of operation
     */
    Status Start(const std::string& address);

    /**
     * @brief Stop gRPC server
     * @return Status of operation
     */
    Status Stop();

    /**
     * @brief Wait for server to finish
     */
    void Wait();

    /**
     * @brief Get KV store
     * @return Pointer to KV store
     */
    KVStore* GetKVStore() { return &kvStore_; }

    /**
     * @brief Get lease manager
     * @return Pointer to lease manager
     */
    LeaseManager* GetLeaseManager() { return &leaseManager_; }

    /**
     * @brief Get watch manager
     * @return Pointer to watch manager
     */
    WatchManager* GetWatchManager() { return &watchManager_; }

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool IsRunning() const { return server_ != nullptr; }

private:
    std::unique_ptr<grpc::Server> server_;
    KVStore kvStore_;
    LeaseManager leaseManager_;
    WatchManager watchManager_;

    std::unique_ptr<KVServiceImpl> kvService_;
    std::unique_ptr<LeaseServiceImpl> leaseService_;
    std::unique_ptr<WatchServiceImpl> watchService_;
    std::unique_ptr<MaintenanceServiceImpl> maintenanceService_;
};

}  // namespace etcdlite

#endif  // ETCDLITE_SERVER_H
