// Copyright 2024 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test/core/transport/test_suite/chaotic_good_fixture_helpers.h"

using grpc_event_engine::experimental::EventEngine;

namespace grpc_core {

TRANSPORT_FIXTURE(ChaoticGoodSingleConnection) {
  auto resource_quota = MakeResourceQuota("test");
  EndpointPair control_endpoints =
      CreateEndpointPair(event_engine.get(), resource_quota.get(), 1234);
  auto channel_args =
      ChannelArgs()
          .SetObject(resource_quota)
          .SetObject(
              std::static_pointer_cast<
                  grpc_event_engine::experimental::EventEngine>(event_engine));
  chaotic_good::Config client_config(channel_args);
  chaotic_good::Config server_config(channel_args);
  auto client_transport =
      MakeOrphanable<chaotic_good::ChaoticGoodClientTransport>(
          channel_args,
          MakeOrphanable<chaotic_good::TcpFrameTransport>(
              client_config.MakeTcpFrameTransportOptions(),
              std::move(control_endpoints.client),
              client_config.TakePendingDataEndpoints(),
              MakeRefCounted<chaotic_good::TransportContext>(channel_args)),
          client_config.MakeMessageChunker());
  auto server_transport =
      MakeOrphanable<chaotic_good::ChaoticGoodServerTransport>(
          channel_args,
          MakeOrphanable<chaotic_good::TcpFrameTransport>(
              server_config.MakeTcpFrameTransportOptions(),
              std::move(control_endpoints.server),
              server_config.TakePendingDataEndpoints(),
              MakeRefCounted<chaotic_good::TransportContext>(channel_args)),
          server_config.MakeMessageChunker());
  return ClientAndServerTransportPair{std::move(client_transport),
                                      std::move(server_transport)};
}

}  // namespace grpc_core
