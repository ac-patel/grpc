//
//
// Copyright 2015 gRPC authors.
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
//

#include <grpc/credentials.h>
#include <grpc/grpc_security.h>
#include <grpc/grpc_security_constants.h>
#include <grpc/support/alloc.h>
#include <grpc/support/port_platform.h>
#include <grpc/support/string_util.h>
#include <string.h>

#include <functional>
#include <memory>
#include <type_traits>  // IWYU pragma: keep
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/core/call/metadata_batch.h"
#include "src/core/call/security_context.h"
#include "src/core/call/status_util.h"
#include "src/core/credentials/call/call_credentials.h"
#include "src/core/credentials/transport/security_connector.h"
#include "src/core/filter/auth/auth_filters.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/channel/channel_fwd.h"
#include "src/core/lib/channel/channel_stack.h"
#include "src/core/lib/channel/promise_based_filter.h"
#include "src/core/lib/promise/arena_promise.h"
#include "src/core/lib/promise/context.h"
#include "src/core/lib/promise/promise.h"
#include "src/core/lib/promise/seq.h"
#include "src/core/lib/promise/try_seq.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/lib/transport/transport.h"
#include "src/core/transport/auth_context.h"
#include "src/core/util/debug_location.h"
#include "src/core/util/ref_counted_ptr.h"

#define MAX_CREDENTIALS_METADATA_COUNT 4

void grpc_auth_metadata_context_copy(grpc_auth_metadata_context* from,
                                     grpc_auth_metadata_context* to) {
  grpc_auth_metadata_context_reset(to);
  to->channel_auth_context = from->channel_auth_context;
  if (to->channel_auth_context != nullptr) {
    const_cast<grpc_auth_context*>(to->channel_auth_context)
        ->Ref(DEBUG_LOCATION, "grpc_auth_metadata_context_copy")
        .release();
  }
  to->service_url = gpr_strdup(from->service_url);
  to->method_name = gpr_strdup(from->method_name);
}

void grpc_auth_metadata_context_reset(
    grpc_auth_metadata_context* auth_md_context) {
  if (auth_md_context->service_url != nullptr) {
    gpr_free(const_cast<char*>(auth_md_context->service_url));
    auth_md_context->service_url = nullptr;
  }
  if (auth_md_context->method_name != nullptr) {
    gpr_free(const_cast<char*>(auth_md_context->method_name));
    auth_md_context->method_name = nullptr;
  }
  if (auth_md_context->channel_auth_context != nullptr) {
    const_cast<grpc_auth_context*>(auth_md_context->channel_auth_context)
        ->Unref(DEBUG_LOCATION, "grpc_auth_metadata_context");
    auth_md_context->channel_auth_context = nullptr;
  }
}

static grpc_security_level convert_security_level_string_to_enum(
    const char* security_level) {
  if (strcmp(security_level, "TSI_INTEGRITY_ONLY") == 0) {
    return GRPC_INTEGRITY_ONLY;
  } else if (strcmp(security_level, "TSI_PRIVACY_AND_INTEGRITY") == 0) {
    return GRPC_PRIVACY_AND_INTEGRITY;
  }
  return GRPC_SECURITY_NONE;
}

bool grpc_check_security_level(grpc_security_level channel_level,
                               grpc_security_level call_cred_level) {
  return static_cast<int>(channel_level) >= static_cast<int>(call_cred_level);
}

namespace grpc_core {

// ClientAuthFilter

ClientAuthFilter::ClientAuthFilter(
    RefCountedPtr<grpc_channel_security_connector> security_connector,
    RefCountedPtr<grpc_auth_context> auth_context)
    : args_{std::move(security_connector), std::move(auth_context)} {}

absl::StatusOr<RefCountedPtr<grpc_call_credentials>>
ClientAuthFilter::GetCallCreds() {
  auto* ctx = GetContext<grpc_client_security_context>();
  grpc_call_credentials* channel_call_creds =
      args_.security_connector->mutable_request_metadata_creds();
  const bool call_creds_has_md = (ctx != nullptr) && (ctx->creds != nullptr);

  if (channel_call_creds == nullptr && !call_creds_has_md) {
    // Skip sending metadata altogether.
    return nullptr;
  }

  RefCountedPtr<grpc_call_credentials> creds;
  if (channel_call_creds != nullptr && call_creds_has_md) {
    creds = RefCountedPtr<grpc_call_credentials>(
        grpc_composite_call_credentials_create(channel_call_creds,
                                               ctx->creds.get(), nullptr));
    if (creds == nullptr) {
      return absl::UnauthenticatedError(
          "Incompatible credentials set on channel and call.");
    }
  } else if (call_creds_has_md) {
    creds = ctx->creds->Ref();
  } else {
    creds = channel_call_creds->Ref();
  }

  // Check security level of call credential and channel, and do not send
  // metadata if the check fails.
  grpc_auth_property_iterator it = grpc_auth_context_find_properties_by_name(
      args_.auth_context.get(), GRPC_TRANSPORT_SECURITY_LEVEL_PROPERTY_NAME);
  const grpc_auth_property* prop = grpc_auth_property_iterator_next(&it);
  if (prop == nullptr) {
    return absl::UnauthenticatedError(
        "Established channel does not have an auth "
        "property representing a security level.");
  }
  const grpc_security_level call_cred_security_level =
      creds->min_security_level();
  const bool is_security_level_ok = grpc_check_security_level(
      convert_security_level_string_to_enum(prop->value),
      call_cred_security_level);
  if (!is_security_level_ok) {
    return absl::UnauthenticatedError(
        "Established channel does not have a sufficient security level to "
        "transfer call credential.");
  }

  return std::move(creds);
}

void ClientAuthFilter::InstallContext() {
  auto* sec_ctx = MaybeGetContext<grpc_client_security_context>();
  if (sec_ctx == nullptr) {
    sec_ctx = grpc_client_security_context_create(GetContext<Arena>(),
                                                  /*creds=*/nullptr);
    SetContext<SecurityContext>(sec_ctx);
  }
  sec_ctx->auth_context = args_.auth_context;
}

absl::StatusOr<std::unique_ptr<ClientAuthFilter>> ClientAuthFilter::Create(
    const ChannelArgs& args, ChannelFilter::Args) {
  auto* sc = args.GetObject<grpc_security_connector>();
  if (sc == nullptr) {
    return absl::InvalidArgumentError(
        "Security connector missing from client auth filter args");
  }
  auto* auth_context = args.GetObject<grpc_auth_context>();
  if (auth_context == nullptr) {
    return absl::InvalidArgumentError(
        "Auth context missing from client auth filter args");
  }
  return std::make_unique<ClientAuthFilter>(
      sc->RefAsSubclass<grpc_channel_security_connector>(),
      auth_context->Ref());
}

const grpc_channel_filter ClientAuthFilter::kFilter =
    MakePromiseBasedFilter<ClientAuthFilter, FilterEndpoint::kClient>();

}  // namespace grpc_core
