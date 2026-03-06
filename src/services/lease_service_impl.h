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
 * Description: Lease gRPC service implementation for etcd-lite
 */
#ifndef ETCDLITE_LEASE_SERVICE_IMPL_H
#define ETCDLITE_LEASE_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>
#include <memory>

#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "lease/lease_manager.h"
#include "kv/kv_store.h"

namespace etcdlite {

class LeaseServiceImpl : public etcdserverpb::Lease::Service {
public:
    explicit LeaseServiceImpl(LeaseManager* leaseManager, KVStore* kvStore);
    ~LeaseServiceImpl() = default;

    ::grpc::Status LeaseGrant(::grpc::ServerContext* context,
                               const ::etcdserverpb::LeaseGrantRequest* request,
                               ::etcdserverpb::LeaseGrantResponse* response) override;

    ::grpc::Status LeaseKeepAlive(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::etcdserverpb::LeaseKeepAliveResponse, ::etcdserverpb::LeaseKeepAliveRequest>* stream) override;

    ::grpc::Status LeaseRevoke(::grpc::ServerContext* context,
                               const ::etcdserverpb::LeaseRevokeRequest* request,
                               ::etcdserverpb::LeaseRevokeResponse* response) override;

    ::grpc::Status LeaseTimeToLive(::grpc::ServerContext* context,
                               const ::etcdserverpb::LeaseTimeToLiveRequest* request,
                               ::etcdserverpb::LeaseTimeToLiveResponse* response) override;

private:
    LeaseManager* leaseManager_;
    KVStore* kvStore_;

    void FillResponseHeader(::etcdserverpb::ResponseHeader* header);
};

}  // namespace etcdlite

#endif  // ETCDLITE_LEASE_SERVICE_IMPL_H
