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
 * Description: Status class for error handling
 */
#ifndef ETCDLITE_STATUS_H
#define ETCDLITE_STATUS_H

#include <string>

namespace etcdlite {

enum class StatusCode {
    OK = 0,
    INVALID_ARGUMENT = 1,
    NOT_FOUND = 2,
    ALREADY_EXISTS = 3,
    FAILED_PRECONDITION = 4,
    OUT_OF_RANGE = 5,
    INTERNAL = 6,
    UNIMPLEMENTED = 7,
    UNKNOWN = 8
};

class Status {
public:
    Status() : code_(StatusCode::OK), message_("") {}
    Status(StatusCode code, const std::string& message)
        : code_(code), message_(message) {}

    bool IsOk() const { return code_ == StatusCode::OK; }
    bool IsError() const { return code_ != StatusCode::OK; }

    StatusCode Code() const { return code_; }
    const std::string& Message() const { return message_; }

    static Status OK() { return Status(); }

private:
    StatusCode code_;
    std::string message_;
};

}  // namespace etcdlite

#endif  // ETCDLITE_STATUS_H
