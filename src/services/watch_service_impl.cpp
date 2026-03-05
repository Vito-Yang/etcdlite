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
 * Description: Watch gRPC service implementation for etcd-lite
 */
#include "services/watch_service_impl.h"

namespace etcdlite {

WatchServiceImpl::WatchServiceImpl(WatchManager* watchManager, KVStore* kvStore)
    : watchManager_(watchManager), kvStore_(kvStore)
{
}

WatchServiceImpl::~WatchServiceImpl()
{
}

void WatchServiceImpl::FillResponseHeader(::etcdserverpb::ResponseHeader* header)
{
    header->set_cluster_id(kvStore_->GetClusterId());
    header->set_member_id(kvStore_->GetMemberId());
    header->set_revision(kvStore_->CurrentRevision());
    header->set_raft_term(1);
}

::grpc::Status WatchServiceImpl::Watch(
    ::grpc::ServerContext* context,
    ::grpc::ServerReaderWriter<::etcdserverpb::WatchResponse, ::etcdserverpb::WatchRequest>* stream)
{
    (void)context;

    // 处理stream请求，直到客户端断开连接
    ::etcdserverpb::WatchRequest req;
    while (stream->Read(&req)) {
        if (req.has_create_request()) {
            int64_t watchId = 0;
            Status status = watchManager_->RegisterWatcher(
                req.create_request(), stream, &watchId, kvStore_);
            if (status.IsError()) {
                // 发送错误响应
                ::etcdserverpb::WatchResponse resp;
                auto* header = resp.mutable_header();
                FillResponseHeader(header);
                resp.set_watch_id(watchId);
                resp.set_canceled(true);
                stream->Write(resp);
                return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.Message());
            }
        } else if (req.has_cancel_request()) {
            watchManager_->CancelWatcher(req.cancel_request().watch_id());
        }
        // progress_request: 忽略，不做处理
    }

    // 客户端断开连接，自然退出
    return ::grpc::Status::OK;
}

}  // namespace etcdlite
