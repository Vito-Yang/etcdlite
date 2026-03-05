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
 * Description: EtcdLite - A lightweight etcd gRPC server implementation
 * This header provides the main EtcdLiteServer class that implements
 * a subset of the etcd v3 gRPC API.
 *
 * Features:
 * - KV operations (Put, Range, DeleteRange, Txn)
 * - Watch operations (stream-based)
 * - Lease operations (Grant, KeepAlive, Revoke)
 * - Maintenance operations (Status for health check)
 *
 * Usage:
 *   #include "etcdlite.h"
 *
 *   etcdlite::EtcdLiteServer server;
 *   server.Start("0.0.0.0:2379");
 *   server.Wait();
 */
#ifndef ETCDLITE_ETCDLITE_H
#define ETCDLITE_ETCDLITE_H

#include "server.h"

#endif  // ETCDLITE_ETCDLITE_H
