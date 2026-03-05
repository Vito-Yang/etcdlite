/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * Licensed under Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
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
 * Description: Main entry point for etcd-lite server
 */
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

#include "server.h"

using namespace etcdlite;

std::unique_ptr<EtcdLiteServer> g_server;
std::atomic<bool> g_shutdown(false);
std::thread g_shutdown_thread;

void SignalHandler(int signal)
{
    (void)signal;
    std::cout << "\nReceived signal, shutting down..." << std::endl;
    g_shutdown.store(true);

    // Don't call Stop() directly in signal handler as it can cause deadlock
    // with gRPC's Wait(). Instead, let the shutdown thread handle it.
}

void ShutdownThread()
{
    // Wait a bit for shutdown signal
    while (!g_shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Now safe to call Stop() - wait() should be interrupted or we're in safe context
    if (g_server) {
        g_server->Stop();
    }
}

int main(int argc, char** argv)
{
    std::string address = "0.0.0.0:2379";
    if (argc > 1) {
        address = argv[1];
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    g_server = std::make_unique<EtcdLiteServer>();
    Status status = g_server->Start(address);
    if (!status.IsOk()) {
        std::cerr << "Failed to start server: " << status.Message() << std::endl;
        return 1;
    }

    std::cout << "EtcdLite server started on " << address << std::endl;

    // Start shutdown thread
    g_shutdown_thread = std::thread(ShutdownThread);

    // Wait for server to finish
    g_server->Wait();

    // Wait for shutdown thread to complete
    if (g_shutdown_thread.joinable()) {
        g_shutdown_thread.join();
    }

    std::cout << "EtcdLite server stopped" << std::endl;

    return 0;
}
