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
#include "server.h"

#include <iostream>

namespace etcdlite {

EtcdLiteServer::EtcdLiteServer()
{
    // Set lease manager for KVStore
    kvStore_.SetLeaseManager(&leaseManager_);

    // Setup watch callbacks in KVStore
    kvStore_.SetWatchCallback(
        [this](const std::string& key, const mvccpb::KeyValue& kv, const mvccpb::KeyValue* prevKv) {
            watchManager_.NotifyPut(key, kv, prevKv, kvStore_.CurrentRevision());
        },
        [this](const std::string& key, const mvccpb::KeyValue& kv) {
            watchManager_.NotifyDelete(key, kv, kvStore_.CurrentRevision());
        }
    );

    // Setup delete key callback in LeaseManager
    leaseManager_.SetDeleteKeyCallback(
        [this](const std::string& key) {
            return kvStore_.Delete(key, "", nullptr);
        }
    );

    // Create g gRPC services
    kvService_ = std::make_unique<KVServiceImpl>(&kvStore_, &leaseManager_);
    leaseService_ = std::make_unique<LeaseServiceImpl>(&leaseManager_, &kvStore_);
    watchService_ = std::make_unique<WatchServiceImpl>(&watchManager_, &kvStore_);
    maintenanceService_ = std::make_unique<MaintenanceServiceImpl>(&kvStore_);
}

EtcdLiteServer::~EtcdLiteServer()
{
    Stop();
}

Status EtcdLiteServer::Start(const std::string& address)
{
    // Create gRPC server builder
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());


    // Configure gRPC server keepalive settings to prevent "too_many_pings" errors
    // These settings allow more frequent client ping messages
    builder.SetMaxReceiveMessageSize(4 * 1024 * 1024);
    builder.SetMaxSendMessageSize(4 * 1024 * 1024);

    // Allow client to send pings more frequently
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 1000);  // 1 second
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 60000);  // 60 seconds
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20000);  // 20 seconds
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

    // Register services
    builder.RegisterService(kvService_.get());
    builder.RegisterService(leaseService_.get());
    builder.RegisterService(watchService_.get());
    builder.RegisterService(maintenanceService_.get());

    // Build and start server
    server_ = builder.BuildAndStart();
    if (!server_) {
        return Status(StatusCode::INTERNAL, "Failed to build gRPC server");
    }

    // Start lease expiration check
    leaseManager_.StartExpirationCheck();

    std::cout << "EtcdLite server started on " << address << std::endl;
    return Status::OK();
}

Status EtcdLiteServer::Stop()
{
    if (server_) {
        std::cout << "Shutting down EtcdLite server..." << std::endl;

        // Stop lease expiration check
        leaseManager_.StopExpirationCheck();

        // Shutdown gRPC server
        server_->Shutdown();
        server_->Wait();
        server_.reset();
    }

    return Status::OK();
}

void EtcdLiteServer::Wait()
{
    if (server_) {
        server_->Wait();
    }
}

}  // namespace etcdlite
