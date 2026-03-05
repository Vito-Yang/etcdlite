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
#include "services/lease_service_impl.h"

namespace etcdlite {

LeaseServiceImpl::LeaseServiceImpl(LeaseManager* leaseManager, KVStore* kvStore)
    : leaseManager_(leaseManager), kvStore_(kvStore)
{
}

void LeaseServiceImpl::FillResponseHeader(::etcdserverpb::ResponseHeader* header)
{
    header->set_cluster_id(kvStore_->GetClusterId());
    header->set_member_id(kvStore_->GetMemberId());
    header->set_revision(kvStore_->CurrentRevision());
    header->set_raft_term(1);
}

::grpc::Status LeaseServiceImpl::LeaseGrant(
    ::grpc::ServerContext* context,
    const ::etcdserverpb::LeaseGrantRequest* request,
    ::etcdserverpb::LeaseGrantResponse* response)
{
    (void)context;

    int64_t leaseId = 0;
    int64_t actualTtl = 0;
    Status status = leaseManager_->Grant(request->ttl(), &leaseId, &actualTtl);
    if (status.IsError()) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
    }

    FillResponseHeader(response->mutable_header());
    response->set_id(leaseId);
    response->set_ttl(actualTtl);

    return ::grpc::Status::OK;
}

::grpc::Status LeaseServiceImpl::LeaseKeepAlive(
    ::grpc::ServerContext* context,
    ::grpc::ServerReaderWriter<::etcdserverpb::LeaseKeepAliveResponse, ::etcdserverpb::LeaseKeepAliveRequest>* stream)
{
    (void)context;

    ::etcdserverpb::LeaseKeepAliveRequest req;
    while (stream->Read(&req)) {
        ::etcdserverpb::LeaseKeepAliveResponse resp;
        int64_t ttl = 0;
        Status status = leaseManager_->KeepAlive(req.id(), &ttl);
        if (status.IsError()) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
        }

        FillResponseHeader(resp.mutable_header());
        resp.set_id(req.id());
        resp.set_ttl(ttl);

        if (!stream->Write(resp)) {
            break;
        }
    }

    return ::grpc::Status::OK;
}

}  // namespace etcdlite
