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
#include "services/maintenance_service_impl.h"

namespace etcdlite {

MaintenanceServiceImpl::MaintenanceServiceImpl(KVStore* kvStore)
    : kvStore_(kvStore)
{
}

::grpc::Status MaintenanceServiceImpl::Status(
    ::grpc::ServerContext* context,
    const ::etcdserverpb::StatusRequest* request,
    ::etcdserverpb::StatusResponse* response)
{
    (void)context;
    (void)request;

    auto* header = response->mutable_header();
    header->set_cluster_id(kvStore_->GetClusterId());
    header->set_member_id(kvStore_->GetMemberId());
    header->set_revision(kvStore_->CurrentRevision());
    header->set_raft_term(1);

    response->set_version("3.5.0");  // etcd 版本
    response->set_dbsize(0);
    response->set_leader(1);  // 默认自己是leader
    response->set_raftindex(kvStore_->CurrentRevision());
    response->set_raftterm(1);
    response->set_raftappliedindex(0);
    response->set_dbsizeinuse(0);
    response->set_islearner(false);

    return ::grpc::Status::OK;
}

}  // namespace etcdlite
