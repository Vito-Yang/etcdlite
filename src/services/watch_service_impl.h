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
#ifndef ETCDLITE_WATCH_SERVICE_IMPL_H
#define ETCDLITE_WATCH_SERVICE_IMPL_H

#include <grpcpp/grpcpp.h>

#include "etcd/api/etcdserverpb/rpc.grpc.pb.h"
#include "watch/watch_manager.h"
#include "kv/kv_store.h"

namespace etcdlite {

class WatchServiceImpl : public etcdserverpb::Watch::Service {
public:
    explicit WatchServiceImpl(WatchManager* watchManager, KVStore* kvStore);
    ~WatchServiceImpl();

    ::grpc::Status Watch(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::etcdserverpb::WatchResponse, ::etcdserverpb::WatchRequest>* stream) override;

private:
    WatchManager* watchManager_;
    KVStore* kvStore_;

    void FillResponseHeader(::etcdserverpb::ResponseHeader* header);
};

}  // namespace etcdlite

#endif  // ETCDLITE_WATCH_SERVICE_IMPL_H
