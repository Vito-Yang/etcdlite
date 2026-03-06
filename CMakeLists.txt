# Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.15)
project(etcdlite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找依赖
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

message(STATUS "Found gRPC: ${gRPC_VERSION}")
message(STATUS "Found Protobuf: ${protobuf_VERSION}")

# Proto 文件目录
set(PROTO_SRC_DIR ${CMAKE_SOURCE_DIR}/third_party/protos)

# 生成 proto - 使用 protoc 命令直接调用以确保正确的 import 路径
set(PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

# 设置 proto 文件列表
set(PROTO_FILES
    ${PROTO_SRC_DIR}/etcd/api/etcdserverpb/rpc.proto
    ${PROTO_SRC_DIR}/etcd/api/mvccpb/kv.proto
    ${PROTO_SRC_DIR}/etcd/api/authpb/auth.proto
)

# 清空输出文件列表
set(PROTO_SRCS "")
set(PROTO_HDRS "")

# 生成 gogoproto/gogo.pb.h (Go-specific extensions, stub for C++)
set(GOGO_PROTO_FILE "${CMAKE_SOURCE_DIR}/third_party/protos/gogoproto/gogo.proto")
set(GOGO_CC "${PROTO_OUTPUT_DIR}/gogoproto/gogo.pb.cc")
set(GOGO_H "${PROTO_OUTPUT_DIR}/gogoproto/gogo.pb.h")

add_custom_command(
    OUTPUT ${GOGO_CC} ${GOGO_H}
    COMMAND protobuf::protoc
    ARGS
        --proto_path=${CMAKE_SOURCE_DIR}/third_party/protos
        --cpp_out=${PROTO_OUTPUT_DIR}
        ${GOGO_PROTO_FILE}
    DEPENDS ${GOGO_PROTO_FILE}
    COMMENT "Generating protobuf for gogoproto/gogo"
)

list(APPEND PROTO_SRCS ${GOGO_CC})
list(APPEND PROTO_HDRS ${GOGO_H})

# 生成 google/api/annotations.proto (grpc-gateway annotations)
set(GOOGLE_ANNOTATIONS_PROTO_FILE "${CMAKE_SOURCE_DIR}/third_party/protos/google/api/annotations.proto")
set(GOOGLE_ANNOTATIONS_CC "${PROTO_OUTPUT_DIR}/google/api/annotations.pb.cc")
set(GOOGLE_ANNOTATIONS_H "${PROTO_OUTPUT_DIR}/google/api/annotations.pb.h")

add_custom_command(
    OUTPUT ${GOOGLE_ANNOTATIONS_CC} ${GOOGLE_ANNOTATIONS_H}
    COMMAND protobuf::protoc
    ARGS
        --proto_path=${CMAKE_SOURCE_DIR}/third_party/protos
        --cpp_out=${PROTO_OUTPUT_DIR}
        ${GOOGLE_ANNOTATIONS_PROTO_FILE}
    DEPENDS ${GOOGLE_ANNOTATIONS_PROTO_FILE}
    COMMENT "Generating protobuf for google/api/annotations"
)

list(APPEND PROTO_SRCS ${GOOGLE_ANNOTATIONS_CC})
list(APPEND PROTO_HDRS ${GOOGLE_ANNOTATIONS_H})

# 生成 google/api/http.proto (grpc-gateway http)
set(GOOGLE_HTTP_PROTO_FILE "${CMAKE_SOURCE_DIR}/third_party/protos/google/api/http.proto")
set(GOOGLE_HTTP_CC "${PROTO_OUTPUT_DIR}/google/api/http.pb.cc")
set(GOOGLE_HTTP_H "${PROTO_OUTPUT_DIR}/google/api/http.pb.h")

add_custom_command(
    OUTPUT ${GOOGLE_HTTP_CC} ${GOOGLE_HTTP_H}
    COMMAND protobuf::protoc
    ARGS
        --proto_path=${CMAKE_SOURCE_DIR}/third_party/protos
        --proto_path=/usr/include
        --cpp_out=${PROTO_OUTPUT_DIR}
        ${GOOGLE_HTTP_PROTO_FILE}
    DEPENDS ${GOOGLE_HTTP_PROTO_FILE}
    COMMENT "Generating protobuf for google/api/http"
)

list(APPEND PROTO_SRCS ${GOOGLE_HTTP_CC})
list(APPEND PROTO_HDRS ${GOOGLE_HTTP_H})

# 为每个 proto 文件生成 .pb.cc 和 .pb.h
# 注意：protoc 会保持源文件的目录结构
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
    # 获取相对于 PROTO_SRC_DIR 的相对路径
    string(REPLACE ${PROTO_SRC_DIR}/ "" PROTO_REL_DIR ${PROTO_DIR})

    set(PROTO_CC "${PROTO_OUTPUT_DIR}/${PROTO_REL_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_H "${PROTO_OUTPUT_DIR}/${PROTO_REL_DIR}/${PROTO_NAME}.pb.h")

    add_custom_command(
        OUTPUT ${PROTO_CC} ${PROTO_H}
        COMMAND protobuf::protoc
        ARGS
            --proto_path=${PROTO_SRC_DIR}
            --cpp_out=${PROTO_OUTPUT_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf for ${PROTO_NAME}"
    )

    list(APPEND PROTO_SRCS ${PROTO_CC})
    list(APPEND PROTO_HDRS ${PROTO_H})
endforeach()

# 生成 gRPC 文件
set(GRPC_PROTO_FILE ${PROTO_SRC_DIR}/etcd/api/etcdserverpb/rpc.proto)
set(GGRPC_CC "${PROTO_OUTPUT_DIR}/etcd/api/etcdserverpb/rpc.grpc.pb.cc")
set(GGRPC_H "${PROTO_OUTPUT_DIR}/etcd/api/etcdserverpb/rpc.grpc.pb.h")

add_custom_command(
    OUTPUT ${GGRPC_CC} ${GGRPC_H}
    COMMAND protobuf::protoc
    ARGS
        --proto_path=${PROTO_SRC_DIR}
        --grpc_out=${PROTO_OUTPUT_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${GRPC_PROTO_FILE}
    DEPENDS ${GRPC_PROTO_FILE}
    COMMENT "Generating gRPC for rpc.proto"
)

list(APPEND PROTO_SRCS ${GGRPC_CC})
list(APPEND PROTO_HDRS ${GGRPC_H})

# 创建 proto 库
add_library(etcdlite_proto ${PROTO_SRCS})
target_include_directories(etcdlite_proto PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party
    ${PROTO_OUTPUT_DIR})
# 为静态库添加 PIC 支持，使其可以链接到共享库
set_target_properties(etcdlite_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 主库源文件
set(ETCDLITE_SRCS
    src/server.cpp
    src/status.cpp
    src/kv/kv_store.cpp
    src/lease/lease_manager.cpp
    src/watch/watch_manager.cpp
    src/services/kv_service_impl.cpp
    src/services/lease_service_impl.cpp
    src/services/watch_service_impl.cpp
    src/services/maintenance_service_impl.cpp
)

# 头文件
set(ETCDLITE_HDRS
    src/etcdlite.h
    src/server.h
    src/status.h
    src/kv/kv_store.h
    src/lease/lease_manager.h
    src/watch/watch_manager.h
    src/services/kv_service_impl.h
    src/services/lease_service_impl.h
    src/services/watch_service_impl.h
    src/services/maintenance_service_impl.h
)

# 创建主库
add_library(etcdlite SHARED ${ETCDLITE_SRCS})
target_sources(etcdlite PUBLIC ${ETCDLITE_HDRS})
target_include_directories(etcdlite PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(etcdlite PUBLIC
    etcdlite_proto
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    pthread
)

# 可执行文件
add_executable(etcdlite-server src/main.cpp)
target_link_libraries(etcdlite-server PRIVATE etcdlite)

# 创建直接测试程序
add_executable(test_lease_direct test_lease_direct.cpp)
target_include_directories(test_lease_direct PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_lease_direct
    PRIVATE etcdlite
    PRIVATE etcdlite_proto
    PRIVATE pthread
)

# 安装目标
install(TARGETS etcdlite etcdlite-server DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)


# 启用测试
include(CTest)
enable_testing()

# 下载并配置 Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# 定义测试目标
set(TEST_SRCS
    test/test_kv_store.cpp
    test/test_lease_manager.cpp
    test/test_watch_manager.cpp
    test/test_maintenance_service.cpp
)

# 创建测试可执行文件
add_executable(etcdlite_tests ${TEST_SRCS})

# 配置测试可执行文件的编译选项
target_include_directories(etcdlite_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
)

# 链接库
target_link_libraries(etcdlite_tests
    PRIVATE etcdlite
    PRIVATE etcdlite_proto
    PRIVATE GTest::gtest
    PRIVATE GTest::gtest_main
    PRIVATE pthread
)

# 将测试添加到 CTest
include(GoogleTest)
gtest_discover_tests(etcdlite_tests)