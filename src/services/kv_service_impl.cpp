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
#include "services/kv_service_impl.h"

namespace etcdlite {

KVServiceImpl::KVServiceImpl(KVStore* kvStore, LeaseManager* leaseManager)
    : kvStore_(kvStore), leaseManager_(leaseManager)
{
}

void KVServiceImpl::FillResponseHeader(::etcdserverpb::ResponseHeader* header)
{
    header->set_cluster_id(kvStore_->GetClusterId());
    header->set_member_id(kvStore_->GetMemberId());
    header->set_revision(kvStore_->CurrentRevision());
    header->set_raft_term(1);
}

::grpc::Status KVServiceImpl::Put(::grpc::ServerContext* context,
                               const ::etcdserverpb::PutRequest* request,
                               ::etcdserverpb::PutResponse* response)
{
    (void)context;

    mvccpb::KeyValue prevKv;
    Status status = kvStore_->Put(request->key(), request->value(), request->lease(),
                                  request->prev_kv() ? &prevKv : nullptr);
    if (status.IsError()) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
    }

    FillResponseHeader(response->mutable_header());
    if (request->prev_kv()) {
        *response->mutable_prev_kv() = prevKv;
    }

    return ::grpc::Status::OK;
}

::grpc::Status KVServiceImpl::Range(::grpc::ServerContext* context,
                                  const ::etcdserverpb::RangeRequest* request,
                                  ::etcdserverpb::RangeResponse* response)
{
    (void)context;

    std::vector<mvccpb::KeyValue> kvs;
    Status status = kvStore_->Range(request->key(), request->range_end(), &kvs,
                                  request->limit(), request->count_only(),
                                  request->keys_only());
    if (status.IsError()) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
    }

    FillResponseHeader(response->mutable_header());
    for (const auto& kv : kvs) {
        *response->add_kvs() = kv;
    }
    response->set_count(kvs.size());
    response->set_more(false);

    return ::grpc::Status::OK;
}

::grpc::Status KVServiceImpl::DeleteRange(::grpc::ServerContext* context,
                                     const ::etcdserverpb::DeleteRangeRequest* request,
                                     ::etcdserverpb::DeleteRangeResponse* response)
{
    (void)context;

    std::vector<mvccpb::KeyValue> prevKvs;
    Status status = kvStore_->Delete(request->key(), request->range_end(), &prevKvs);
    if (status.IsError()) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
    }

    FillResponseHeader(response->mutable_header());
    response->set_deleted(prevKvs.size());
    if (request->prev_kv()) {
        for (const auto& kv : prevKvs) {
            *response->add_prev_kvs() = kv;
        }
    }

    return ::grpc::Status::OK;
}

::grpc::Status KVServiceImpl::Txn(::grpc::ServerContext* context,
                               const ::etcdserverpb::TxnRequest* request,
                               ::etcdserverpb::TxnResponse* response)
{
    (void)context;

    std::vector<etcdserverpb::Compare> compares;
    std::vector<etcdserverpb::RequestOp> success;
    std::vector<etcdserverpb::RequestOp> failure;

    for (const auto& cmp : request->compare()) {
        compares.push_back(cmp);
    }
    for (const auto& op : request->success()) {
        success.push_back(op);
    }
    for (const auto& op : request->failure()) {
        failure.push_back(op);
    }

    Status status = kvStore_->Txn(compares, success, failure, response);
    if (status.IsError()) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
    }

    return ::grpc::Status::OK;
}

::grpc::Status KVServiceImpl::Compact(::grpc::ServerContext* context,
                                   const ::etcdserverpb::CompactionRequest* request,
                                   ::etcdserverpb::CompactionResponse* response)
{
    (void)context;
    (void)request;

    // 压缩是可选的，这里返回成功
    FillResponseHeader(response->mutable_header());
    return ::grpc::Status::OK;
}

}  // namespace etcdlite
