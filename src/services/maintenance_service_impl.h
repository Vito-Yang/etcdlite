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
 * Description: Maintenance gRPC service implementation for etcd-lite
 */
#ifndef ETCDLITE_MAINTENANCE_SERVICE_IMPL_H
#define ETCDLITE_MAINTENANCE_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>

#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "kv/kv_store.h"

namespace etcdlite {

class MaintenanceServiceImpl : public etcdserverpb::Maintenance::Service {
public:
    explicit MaintenanceServiceImpl(KVStore* kvStore);
    ~MaintenanceServiceImpl() = default;

    ::grpc::Status Status(::grpc::ServerContext* context,
                          const ::etcdserverpb::StatusRequest* request,
                          ::etcdserverpb::StatusResponse* response) override;

private:
    KVStore* kvStore_;
};

}  // namespace etcdlite

#endif  // ETCDLITE_MAINTENANCE_SERVICE_IMPL_H
