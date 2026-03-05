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
 * Description: KV gRPC service implementation for etcd-lite
 */
#ifndef ETCDLITE_KV_SERVICE_IMPL_H
#define ETCDLITE_KV_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>
#include <memory>

#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "kv/kv_store.h"
#include "lease/lease_manager.h"
#include "watch/watch_manager.h"

namespace etcdlite {

class KVServiceImpl : public etcdserverpb::KV::Service {
public:
    explicit KVServiceImpl(KVStore* kvStore, LeaseManager* leaseManager);
    ~KVServiceImpl() = default;

    ::grpc::Status Put(::grpc::ServerContext* context,
                       const ::etcdserverpb::PutRequest* request,
                       ::etcdserverpb::PutResponse* response) override;

    ::grpc::Status Range(::grpc::ServerContext* context,
                          const ::etcdserverpb::RangeRequest* request,
                          ::etcdserverpb::RangeResponse* response) override;

    ::grpc::Status DeleteRange(::grpc::ServerContext* context,
                             const ::etcdserverpb::DeleteRangeRequest* request,
                             ::etcdserverpb::DeleteRangeResponse* response) override;

    ::grpc::Status Txn(::grpc::ServerContext* context,
                       const ::etcdserverpb::TxnRequest* request,
                       ::etcdserverpb::TxnResponse* response) override;

    ::grpc::Status Compact(::grpc::ServerContext* context,
                          const ::etcdserverpb::CompactionRequest* request,
                          ::etcdserverpb::CompactionResponse* response) override;

private:
    KVStore* kvStore_;
    LeaseManager* leaseManager_;

    void FillResponseHeader(::etcdserverpb::ResponseHeader* header);
};

}  // namespace etcdlite

#endif  // ETCDLITE_KV_SERVICE_IMPL_H
