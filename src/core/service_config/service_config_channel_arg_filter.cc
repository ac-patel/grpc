//
// Copyright 2020 gRPC authors.
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
//

// This filter reads GRPC_ARG_SERVICE_CONFIG and populates ServiceConfigCallData
// in the call context per call for direct channels.

#include "src/core/service_config/service_config_channel_arg_filter.h"

#include <grpc/impl/channel_arg_names.h>
#include <grpc/support/port_platform.h>

#include "src/core/call/metadata_batch.h"
#include "src/core/config/core_configuration.h"
#include "src/core/ext/filters/message_size/message_size_filter.h"
#include "src/core/lib/channel/channel_fwd.h"
#include "src/core/lib/channel/promise_based_filter.h"
#include "src/core/lib/promise/context.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/lib/surface/channel_stack_type.h"
#include "src/core/service_config/service_config.h"
#include "src/core/service_config/service_config_call_data.h"
#include "src/core/service_config/service_config_parser.h"
#include "src/core/util/latent_see.h"
#include "src/core/util/ref_counted_ptr.h"

namespace grpc_core {

void ServiceConfigChannelArgFilter::Call::OnClientInitialMetadata(
    ClientMetadata& md, ServiceConfigChannelArgFilter* filter) {
  GRPC_LATENT_SEE_SCOPE(
      "ServiceConfigChannelArgFilter::Call::OnClientInitialMetadata");
  const ServiceConfigParser::ParsedConfigVector* method_configs = nullptr;
  if (filter->service_config_ != nullptr) {
    method_configs = filter->service_config_->GetMethodParsedConfigVector(
        md.get_pointer(HttpPathMetadata())->c_slice());
  }
  auto* arena = GetContext<Arena>();
  auto* service_config_call_data = arena->New<ServiceConfigCallData>(arena);
  service_config_call_data->SetServiceConfig(filter->service_config_,
                                             method_configs);
}

const grpc_channel_filter ServiceConfigChannelArgFilter::kFilter =
    MakePromiseBasedFilter<ServiceConfigChannelArgFilter,
                           FilterEndpoint::kClient>();

void RegisterServiceConfigChannelArgFilter(
    CoreConfiguration::Builder* builder) {
  builder->channel_init()
      ->RegisterFilter<ServiceConfigChannelArgFilter>(
          GRPC_CLIENT_DIRECT_CHANNEL)
      .ExcludeFromMinimalStack()
      .IfHasChannelArg(GRPC_ARG_SERVICE_CONFIG)
      .Before<ClientMessageSizeFilter>();
}

}  // namespace grpc_core
