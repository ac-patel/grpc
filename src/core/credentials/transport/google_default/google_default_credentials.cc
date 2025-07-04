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

#include "src/core/credentials/transport/google_default/google_default_credentials.h"

#include <grpc/credentials.h>
#include <grpc/grpc_security.h>
#include <grpc/grpc_security_constants.h>
#include <grpc/impl/channel_arg_names.h>
#include <grpc/slice.h>
#include <grpc/support/alloc.h>
#include <grpc/support/port_platform.h>
#include <grpc/support/sync.h>
#include <string.h>

#include <memory>
#include <optional>
#include <string>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "src/core/credentials/call/external/external_account_credentials.h"
#include "src/core/credentials/call/jwt/json_token.h"
#include "src/core/credentials/call/jwt/jwt_credentials.h"
#include "src/core/credentials/call/oauth2/oauth2_credentials.h"
#include "src/core/credentials/transport/alts/alts_security_connector.h"
#include "src/core/credentials/transport/alts/check_gcp_environment.h"
#include "src/core/credentials/transport/transport_credentials.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/debug/trace.h"
#include "src/core/lib/event_engine/shim.h"
#include "src/core/lib/iomgr/closure.h"
#include "src/core/lib/iomgr/error.h"
#include "src/core/lib/iomgr/exec_ctx.h"
#include "src/core/lib/iomgr/iomgr_fwd.h"
#include "src/core/lib/iomgr/polling_entity.h"
#include "src/core/lib/iomgr/pollset.h"
#include "src/core/lib/slice/slice.h"
#include "src/core/lib/slice/slice_internal.h"
#include "src/core/lib/transport/error_utils.h"
#include "src/core/load_balancing/grpclb/grpclb.h"
#include "src/core/load_balancing/xds/xds_channel_args.h"
#include "src/core/util/env.h"
#include "src/core/util/http_client/httpcli.h"
#include "src/core/util/http_client/parser.h"
#include "src/core/util/json/json.h"
#include "src/core/util/json/json_reader.h"
#include "src/core/util/load_file.h"
#include "src/core/util/notification.h"
#include "src/core/util/orphanable.h"
#include "src/core/util/ref_counted_ptr.h"
#include "src/core/util/status_helper.h"
#include "src/core/util/sync.h"
#include "src/core/util/time.h"
#include "src/core/util/uri.h"

using grpc_core::Json;

// -- Constants. --

#define GRPC_COMPUTE_ENGINE_DETECTION_HOST "metadata.google.internal."
#define GRPC_GOOGLE_CREDENTIAL_CREATION_ERROR \
  "Failed to create Google credentials"

// -- Default credentials. --

// A sticky bit that will be set only if the result of metadata server detection
// is positive. We do not set the bit if the result is negative. Because it
// means the detection is done via network test that is unreliable and the
// unreliable result should not be referred by successive calls.
static int g_metadata_server_available = 0;
static grpc_core::Mutex* g_state_mu;
// Protect a metadata_server_detector instance that can be modified by more than
// one gRPC threads
static gpr_mu* g_polling_mu;
static gpr_once g_once = GPR_ONCE_INIT;
static grpc_core::internal::grpc_gce_tenancy_checker g_gce_tenancy_checker =
    grpc_alts_is_running_on_gcp;

static void init_default_credentials(void) {
  g_state_mu = new grpc_core::Mutex();
}

struct metadata_server_detector {
  grpc_polling_entity pollent;
  int is_done;
  int success;
  grpc_http_response response;
  grpc_core::Notification done;
};

namespace {

bool IsXdsNonCfeCluster(std::optional<absl::string_view> xds_cluster) {
  if (!xds_cluster.has_value()) return false;
  if (absl::StartsWith(*xds_cluster, "google_cfe_")) return false;
  if (!absl::StartsWith(*xds_cluster, "xdstp:")) return true;
  auto uri = grpc_core::URI::Parse(*xds_cluster);
  if (!uri.ok()) return true;  // Shouldn't happen, but assume ALTS.
  return uri->authority() != "traffic-director-c2p.xds.googleapis.com" ||
         !absl::StartsWith(uri->path(),
                           "/envoy.config.cluster.v3.Cluster/google_cfe_");
}

}  // namespace

grpc_core::RefCountedPtr<grpc_channel_security_connector>
grpc_google_default_channel_credentials::create_security_connector(
    grpc_core::RefCountedPtr<grpc_call_credentials> call_creds,
    const char* target, grpc_core::ChannelArgs* args) {
  const bool is_grpclb_load_balancer =
      args->GetBool(GRPC_ARG_ADDRESS_IS_GRPCLB_LOAD_BALANCER).value_or(false);
  const bool is_backend_from_grpclb_load_balancer =
      args->GetBool(GRPC_ARG_ADDRESS_IS_BACKEND_FROM_GRPCLB_LOAD_BALANCER)
          .value_or(false);
  const bool is_xds_non_cfe_cluster =
      IsXdsNonCfeCluster(args->GetString(GRPC_ARG_XDS_CLUSTER_NAME));
  const bool use_alts = is_grpclb_load_balancer ||
                        is_backend_from_grpclb_load_balancer ||
                        is_xds_non_cfe_cluster;
  // Return failure if ALTS is selected but not running on GCE.
  if (use_alts && alts_creds_ == nullptr) {
    LOG(ERROR) << "ALTS is selected, but not running on GCE.";
    return nullptr;
  }
  grpc_core::RefCountedPtr<grpc_channel_security_connector> sc =
      use_alts
          ? alts_creds_->create_security_connector(call_creds, target, args)
          : ssl_creds_->create_security_connector(call_creds, target, args);
  // grpclb-specific channel args are removed from the channel args set
  // to ensure backends and fallback addresses will have the same set of channel
  // args. By doing that, it guarantees the connections to backends will not be
  // torn down and re-connected when switching in and out of fallback mode.
  //
  if (use_alts) {
    *args = args->Remove(GRPC_ARG_ADDRESS_IS_GRPCLB_LOAD_BALANCER)
                .Remove(GRPC_ARG_ADDRESS_IS_BACKEND_FROM_GRPCLB_LOAD_BALANCER);
  }
  return sc;
}

grpc_core::ChannelArgs
grpc_google_default_channel_credentials::update_arguments(
    grpc_core::ChannelArgs args) {
  return args.SetIfUnset(GRPC_ARG_DNS_ENABLE_SRV_QUERIES, true);
}

grpc_core::UniqueTypeName grpc_google_default_channel_credentials::Type() {
  static grpc_core::UniqueTypeName::Factory kFactory("GoogleDefault");
  return kFactory.Create();
}

static void on_metadata_server_detection_http_response(
    void* user_data, grpc_error_handle error) {
  metadata_server_detector* detector =
      static_cast<metadata_server_detector*>(user_data);
  if (error.ok() && detector->response.status == 200 &&
      detector->response.hdr_count > 0) {
    // Internet providers can return a generic response to all requests, so
    // it is necessary to check that metadata header is present also.
    size_t i;
    for (i = 0; i < detector->response.hdr_count; i++) {
      grpc_http_header* header = &detector->response.hdrs[i];
      if (strcmp(header->key, "Metadata-Flavor") == 0 &&
          strcmp(header->value, "Google") == 0) {
        detector->success = 1;
        break;
      }
    }
  }
  if (grpc_event_engine::experimental::UsePollsetAlternative()) {
    detector->done.Notify();
    return;
  }
  gpr_mu_lock(g_polling_mu);
  detector->is_done = 1;
  GRPC_LOG_IF_ERROR(
      "Pollset kick",
      grpc_pollset_kick(grpc_polling_entity_pollset(&detector->pollent),
                        nullptr));
  gpr_mu_unlock(g_polling_mu);
}

static void destroy_pollset(void* p, grpc_error_handle /*e*/) {
  grpc_pollset_destroy(static_cast<grpc_pollset*>(p));
}

static int is_metadata_server_reachable() {
  metadata_server_detector detector;
  grpc_http_request request;
  grpc_closure destroy_closure;
  // The http call is local. If it takes more than one sec, it is for sure not
  // on compute engine.
  const auto max_detection_delay = grpc_core::Duration::Seconds(1);
  grpc_pollset* pollset =
      static_cast<grpc_pollset*>(gpr_zalloc(grpc_pollset_size()));
  grpc_pollset_init(pollset, &g_polling_mu);
  detector.pollent = grpc_polling_entity_create_from_pollset(pollset);
  detector.is_done = 0;
  detector.success = 0;
  memset(&request, 0, sizeof(grpc_http_request));
  auto uri = grpc_core::URI::Create("http", /*user_info=*/"",
                                    GRPC_COMPUTE_ENGINE_DETECTION_HOST, "/",
                                    {} /* query params */, "" /* fragment */);
  CHECK(uri.ok());  // params are hardcoded
  auto http_request = grpc_core::HttpRequest::Get(
      std::move(*uri), nullptr /* channel args */, &detector.pollent, &request,
      grpc_core::Timestamp::Now() + max_detection_delay,
      GRPC_CLOSURE_CREATE(on_metadata_server_detection_http_response, &detector,
                          grpc_schedule_on_exec_ctx),
      &detector.response,
      grpc_core::RefCountedPtr<grpc_channel_credentials>(
          grpc_insecure_credentials_create()));
  http_request->Start();
  grpc_core::ExecCtx::Get()->Flush();
  if (grpc_event_engine::experimental::UsePollsetAlternative()) {
    detector.done.WaitForNotification();
  } else {
    // Block until we get the response. This is not ideal but this should only
    // be called once for the lifetime of the process by the default
    // credentials.
    gpr_mu_lock(g_polling_mu);
    while (!detector.is_done) {
      grpc_pollset_worker* worker = nullptr;
      if (!GRPC_LOG_IF_ERROR(
              "pollset_work",
              grpc_pollset_work(grpc_polling_entity_pollset(&detector.pollent),
                                &worker, grpc_core::Timestamp::InfFuture()))) {
        detector.is_done = 1;
        detector.success = 0;
      }
    }
    gpr_mu_unlock(g_polling_mu);
  }
  http_request.reset();
  GRPC_CLOSURE_INIT(&destroy_closure, destroy_pollset,
                    grpc_polling_entity_pollset(&detector.pollent),
                    grpc_schedule_on_exec_ctx);
  grpc_pollset_shutdown(grpc_polling_entity_pollset(&detector.pollent),
                        &destroy_closure);
  g_polling_mu = nullptr;
  grpc_core::ExecCtx::Get()->Flush();
  gpr_free(grpc_polling_entity_pollset(&detector.pollent));
  grpc_http_response_destroy(&detector.response);
  return detector.success;
}

// Takes ownership of creds_path if not NULL.
static grpc_error_handle create_default_creds_from_path(
    const std::string& creds_path,
    grpc_core::RefCountedPtr<grpc_call_credentials>* creds) {
  if (creds_path.empty()) {
    return GRPC_ERROR_CREATE("creds_path unset");
  }
  auto creds_data =
      grpc_core::LoadFile(creds_path, /*add_null_terminator=*/false);
  if (!creds_data.ok()) {
    return absl_status_to_grpc_error(creds_data.status());
  }
  auto json = grpc_core::JsonParse(creds_data->as_string_view());
  if (!json.ok()) {
    return absl_status_to_grpc_error(json.status());
  }
  if (json->type() != Json::Type::kObject) {
    return GRPC_ERROR_CREATE(absl::StrCat("Failed to parse JSON \"",
                                          creds_data->as_string_view(), "\""));
  }
  // First, try an auth json key.
  grpc_auth_json_key key = grpc_auth_json_key_create_from_json(*json);
  if (grpc_auth_json_key_is_valid(&key)) {
    *creds =
        grpc_service_account_jwt_access_credentials_create_from_auth_json_key(
            key, grpc_max_auth_token_lifetime());
    if (*creds == nullptr) {
      return GRPC_ERROR_CREATE(
          "grpc_service_account_jwt_access_credentials_create_from_auth_json_"
          "key failed");
    }
    return absl::OkStatus();
  }
  // Then try a refresh token if the auth json key was invalid.
  grpc_auth_refresh_token token =
      grpc_auth_refresh_token_create_from_json(*json);
  if (grpc_auth_refresh_token_is_valid(&token)) {
    *creds =
        grpc_refresh_token_credentials_create_from_auth_refresh_token(token);
    if (*creds == nullptr) {
      return GRPC_ERROR_CREATE(
          "grpc_refresh_token_credentials_create_from_auth_refresh_token "
          "failed");
    }
    return absl::OkStatus();
  }
  // Use external creds.
  auto external_creds =
      grpc_core::ExternalAccountCredentials::Create(*json, {});
  if (!external_creds.ok()) return external_creds.status();
  *creds = std::move(*external_creds);
  return absl::OkStatus();
}

static void update_tenancy() {
  gpr_once_init(&g_once, init_default_credentials);
  grpc_core::MutexLock lock(g_state_mu);

  // Try a platform-provided hint for GCE.
  if (!g_metadata_server_available) {
    g_metadata_server_available = g_gce_tenancy_checker();
  }
  // TODO(unknown): Add a platform-provided hint for GAE.

  // Do a network test for metadata server.
  if (!g_metadata_server_available) {
    g_metadata_server_available = is_metadata_server_reachable();
  }
}

static bool metadata_server_available() {
  grpc_core::MutexLock lock(g_state_mu);
  return static_cast<bool>(g_metadata_server_available);
}

// A grpc_call_credentials implementation that uses two
// underlying credentials: one for TLS and one for ALTS.
// The implementation will pick the right credentials based on the auth
// context's GRPC_TRANSPORT_SECURITY_TYPE_PROPERTY_NAME property.
class GoogleDefaultCallCredentialsWrapper : public grpc_call_credentials {
 public:
  GoogleDefaultCallCredentialsWrapper(
      grpc_core::RefCountedPtr<grpc_call_credentials> tls_credentials,
      grpc_core::RefCountedPtr<grpc_call_credentials> alts_credentials)
      : tls_credentials_(std::move(tls_credentials)),
        alts_credentials_(std::move(alts_credentials)) {};

  void Orphaned() override {
    tls_credentials_.reset();
    alts_credentials_.reset();
  }

  static grpc_core::UniqueTypeName Type() {
    static grpc_core::UniqueTypeName::Factory kFactory("Dual");
    return kFactory.Create();
  }

  grpc_core::UniqueTypeName type() const override { return Type(); }

  grpc_core::ArenaPromise<absl::StatusOr<grpc_core::ClientMetadataHandle>>
  GetRequestMetadata(grpc_core::ClientMetadataHandle initial_metadata,
                     const GetRequestMetadataArgs* args) override {
    bool use_alts = false;
    if (args != nullptr) {
      auto auth_context = args->auth_context;
      if (auth_context != nullptr &&
          grpc_auth_context_peer_is_authenticated(auth_context.get()) == 1) {
        // This channel is authenticated.
        grpc_auth_property_iterator property_it =
            grpc_auth_context_find_properties_by_name(
                auth_context.get(), GRPC_TRANSPORT_SECURITY_TYPE_PROPERTY_NAME);
        const grpc_auth_property* property =
            grpc_auth_property_iterator_next(&property_it);
        use_alts =
            property != nullptr &&
            strcmp(property->value, GRPC_ALTS_TRANSPORT_SECURITY_TYPE) == 0;
      }
    }
    return (use_alts ? alts_credentials_ : tls_credentials_)
        ->GetRequestMetadata(std::move(initial_metadata), args);
  }

 private:
  int cmp_impl(const grpc_call_credentials* other) const override {
    return QsortCompare(static_cast<const grpc_call_credentials*>(this), other);
  }
  std::string debug_string() override {
    return "GoogleDefaultCallCredentialsWrapper";
  }

  grpc_core::RefCountedPtr<grpc_call_credentials> tls_credentials_;
  grpc_core::RefCountedPtr<grpc_call_credentials> alts_credentials_;
};

static grpc_core::RefCountedPtr<grpc_call_credentials> make_default_call_creds(
    grpc_error_handle* error) {
  grpc_core::RefCountedPtr<grpc_call_credentials> call_creds;
  grpc_error_handle err;

  // First, try the environment variable.
  auto path_from_env = grpc_core::GetEnv(GRPC_GOOGLE_CREDENTIALS_ENV_VAR);
  if (path_from_env.has_value()) {
    err = create_default_creds_from_path(*path_from_env, &call_creds);
    if (err.ok()) return call_creds;
    *error = grpc_error_add_child(*error, err);
  }

  // Then the well-known file.
  err = create_default_creds_from_path(
      grpc_get_well_known_google_credentials_file_path(), &call_creds);
  if (err.ok()) return call_creds;
  *error = grpc_error_add_child(*error, err);

  update_tenancy();

  if (metadata_server_available()) {
    call_creds = grpc_core::RefCountedPtr<grpc_call_credentials>(
        grpc_google_compute_engine_credentials_create(nullptr));
    if (call_creds == nullptr) {
      *error = GRPC_ERROR_CREATE(GRPC_GOOGLE_CREDENTIAL_CREATION_ERROR);
      *error = grpc_error_add_child(
          *error, GRPC_ERROR_CREATE("Failed to get credentials from network"));
    }
  }

  return call_creds;
}

grpc_channel_credentials* grpc_google_default_credentials_create(
    grpc_call_credentials* call_creds_for_tls,
    grpc_call_credentials* call_creds_for_alts) {
  grpc_channel_credentials* result = nullptr;
  grpc_core::RefCountedPtr<grpc_call_credentials> call_creds(
      call_creds_for_tls);
  grpc_error_handle error;
  grpc_core::ExecCtx exec_ctx;

  GRPC_TRACE_LOG(api, INFO)
      << "grpc_google_default_credentials_create(" << call_creds_for_tls << ")";

  if (call_creds == nullptr) {
    call_creds = make_default_call_creds(&error);
  }

  if (call_creds != nullptr) {
    // Create google default credentials.
    grpc_channel_credentials* ssl_creds =
        grpc_ssl_credentials_create(nullptr, nullptr, nullptr, nullptr);
    CHECK_NE(ssl_creds, nullptr);
    grpc_alts_credentials_options* options =
        grpc_alts_credentials_client_options_create();
    grpc_channel_credentials* alts_creds =
        grpc_alts_credentials_create(options);
    grpc_alts_credentials_options_destroy(options);
    auto creds =
        grpc_core::MakeRefCounted<grpc_google_default_channel_credentials>(
            grpc_core::RefCountedPtr<grpc_channel_credentials>(alts_creds),
            grpc_core::RefCountedPtr<grpc_channel_credentials>(ssl_creds));
    if (call_creds_for_alts != nullptr) {
      grpc_core::RefCountedPtr<grpc_call_credentials> alts_call_creds(
          call_creds_for_alts);
      call_creds =
          grpc_core::MakeRefCounted<GoogleDefaultCallCredentialsWrapper>(
              std::move(call_creds), std::move(alts_call_creds));
    }
    result = grpc_composite_channel_credentials_create(
        creds.get(), call_creds.get(), nullptr);
    CHECK_NE(result, nullptr);
  } else {
    LOG(ERROR) << "Could not create google default credentials: "
               << grpc_core::StatusToString(error);
  }
  return result;
}

namespace grpc_core {
namespace internal {
void set_gce_tenancy_checker_for_testing(grpc_gce_tenancy_checker checker) {
  g_gce_tenancy_checker = checker;
}

void grpc_flush_cached_google_default_credentials(void) {
  ExecCtx exec_ctx;
  gpr_once_init(&g_once, init_default_credentials);
  MutexLock lock(g_state_mu);
  g_metadata_server_available = 0;
}

}  // namespace internal
}  // namespace grpc_core

// -- Well known credentials path. --

static grpc_well_known_credentials_path_getter creds_path_getter = nullptr;

std::string grpc_get_well_known_google_credentials_file_path(void) {
  if (creds_path_getter != nullptr) return creds_path_getter();
  return grpc_get_well_known_google_credentials_file_path_impl();
}

void grpc_override_well_known_credentials_path_getter(
    grpc_well_known_credentials_path_getter getter) {
  creds_path_getter = getter;
}
