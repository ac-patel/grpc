# Copyright 2016 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("//bazel:grpc_build_system.bzl", "grpc_cc_test", "grpc_package")
load("//test/core/test_util:grpc_fuzzer.bzl", "grpc_fuzz_test")

grpc_package(name = "test/core/channelz")

licenses(["notice"])

grpc_cc_test(
    name = "channel_trace_test",
    srcs = ["channel_trace_test.cc"],
    external_deps = [
        "gtest",
    ],
    tags = [
        # b/117517566.
        "grpc:no-internal-poller",
        "channelz_test",
    ],
    deps = [
        "//:gpr",
        "//:grpc",
        "//:grpc++",
        "//src/core:channel_args",
        "//test/core/test_util:grpc_test_util",
        "//test/cpp/util:channel_trace_proto_helper",
    ],
)

grpc_cc_test(
    name = "property_list_test",
    srcs = ["property_list_test.cc"],
    external_deps = [
        "gtest",
        "gtest_main",
        "@com_google_protobuf//upb/mem",
        "absl/container:flat_hash_map",
        "absl/log",
        "absl/log:check",
        "absl/strings",
    ],
    deps = [
        "//:channelz_property_list_upb",
        "//src/core:channelz_property_list",
        "//src/core:json_writer",
    ],
)

grpc_cc_test(
    name = "channelz_test",
    srcs = ["channelz_test.cc"],
    external_deps = [
        "gtest",
    ],
    tags = [
        # b/117517566.
        "grpc:no-internal-poller",
        "channelz_test",
    ],
    deps = [
        "//:gpr",
        "//:grpc",
        "//:grpc++",
        "//src/core:channel_args",
        "//src/core:channelz_v2tov1_legacy_api",
        "//test/core/event_engine:event_engine_test_utils",
        "//test/core/test_util:grpc_test_util",
        "//test/cpp/util:channel_trace_proto_helper",
    ],
)

grpc_cc_test(
    name = "channelz_registry_test",
    srcs = ["channelz_registry_test.cc"],
    external_deps = [
        "gtest",
    ],
    shard_count = 25,
    tags = [
        "channelz_test",
    ],
    uses_event_engine = False,
    uses_polling = False,
    deps = [
        "//:gpr",
        "//:grpc",
        "//:grpc++",
        "//test/core/test_util:grpc_test_util",
    ],
)

grpc_cc_test(
    name = "ztrace_collector_test",
    srcs = ["ztrace_collector_test.cc"],
    external_deps = [
        "gtest",
        "gtest_main",
    ],
    tags = [
        "channelz_test",
        "no_windows",
    ],
    deps = [
        "//:grpc",
        "//src/core:notification",
        "//src/core:ztrace_collector",
    ],
)

grpc_fuzz_test(
    name = "channel_trace_fuzzer",
    srcs = ["channel_trace_fuzzer.cc"],
    external_deps = [
        "absl/log:check",
        "fuzztest",
        "fuzztest_main",
    ],
    tags = [
        "channelz_test",
    ],
    deps = [
        "//:channelz",
    ],
)

grpc_fuzz_test(
    name = "channelz_fuzzer",
    srcs = ["channelz_fuzzer.cc"],
    external_deps = [
        "absl/log:check",
        "fuzztest",
        "fuzztest_main",
    ],
    tags = [
        "channelz_test",
    ],
    deps = [
        "//:channelz",
    ],
)
