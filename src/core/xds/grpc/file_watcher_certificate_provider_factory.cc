//
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
//

#include "src/core/xds/grpc/file_watcher_certificate_provider_factory.h"

#include <grpc/support/time.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "src/core/config/core_configuration.h"
#include "src/core/credentials/transport/tls/grpc_tls_certificate_provider.h"
#include "src/core/util/down_cast.h"
#include "src/core/util/env.h"

namespace grpc_core {

namespace {

constexpr absl::string_view kFileWatcherPlugin = "file_watcher";

bool SpiffeBundleMapEnabled() {
  auto value = GetEnv("GRPC_EXPERIMENTAL_XDS_MTLS_SPIFFE");
  if (!value.has_value()) return false;
  bool parsed_value;
  bool parse_succeeded = gpr_parse_bool_value(value->c_str(), &parsed_value);
  return parse_succeeded && parsed_value;
}

}  // namespace

//
// FileWatcherCertificateProviderFactory::Config
//

absl::string_view FileWatcherCertificateProviderFactory::Config::name() const {
  return kFileWatcherPlugin;
}

std::string FileWatcherCertificateProviderFactory::Config::ToString() const {
  std::vector<std::string> parts;
  parts.push_back("{");
  if (!identity_cert_file_.empty()) {
    parts.push_back(
        absl::StrFormat("certificate_file=\"%s\", ", identity_cert_file_));
  }
  if (!private_key_file_.empty()) {
    parts.push_back(
        absl::StrFormat("private_key_file=\"%s\", ", private_key_file_));
  }
  if (!root_cert_file_.empty()) {
    parts.push_back(
        absl::StrFormat("ca_certificate_file=\"%s\", ", root_cert_file_));
  }
  if (!spiffe_bundle_map_file_.empty()) {
    parts.push_back(absl::StrFormat("spiffe_bundle_map_file=\"%s\", ",
                                    spiffe_bundle_map_file_));
  }
  parts.push_back(
      absl::StrFormat("refresh_interval=%ldms}", refresh_interval_.millis()));
  return absl::StrJoin(parts, "");
}

const JsonLoaderInterface*
FileWatcherCertificateProviderFactory::Config::JsonLoader(const JsonArgs&) {
  static const auto* loader =
      JsonObjectLoader<Config>()
          .OptionalField("certificate_file", &Config::identity_cert_file_)
          .OptionalField("private_key_file", &Config::private_key_file_)
          .OptionalField("ca_certificate_file", &Config::root_cert_file_)
          .OptionalField("spiffe_bundle_map_file",
                         &Config::spiffe_bundle_map_file_)
          .OptionalField("refresh_interval", &Config::refresh_interval_)
          .Finish();
  return loader;
}

void FileWatcherCertificateProviderFactory::Config::JsonPostLoad(
    const Json& json, const JsonArgs& /*args*/, ValidationErrors* errors) {
  if ((json.object().find("certificate_file") == json.object().end()) !=
      (json.object().find("private_key_file") == json.object().end())) {
    errors->AddError(
        "fields \"certificate_file\" and \"private_key_file\" must be both set "
        "or both unset");
  }
  if (SpiffeBundleMapEnabled()) {
    if (json.object().find("certificate_file") == json.object().end() &&
        json.object().find("ca_certificate_file") == json.object().end() &&
        json.object().find("spiffe_bundle_map_file") == json.object().end()) {
      errors->AddError(
          "at least one of \"certificate_file\", \"ca_certificate_file\", and "
          "\"spiffe_bundle_map_file\" must be specified");
    }
  } else {
    spiffe_bundle_map_file_ = "";
    if ((json.object().find("certificate_file") == json.object().end()) &&
        (json.object().find("ca_certificate_file") == json.object().end())) {
      errors->AddError(
          "at least one of \"certificate_file\" and \"ca_certificate_file\" "
          "must be specified");
    }
  }
}

//
// FileWatcherCertificateProviderFactory
//

absl::string_view FileWatcherCertificateProviderFactory::name() const {
  return kFileWatcherPlugin;
}

RefCountedPtr<CertificateProviderFactory::Config>
FileWatcherCertificateProviderFactory::CreateCertificateProviderConfig(
    const Json& config_json, const JsonArgs& args, ValidationErrors* errors) {
  return LoadFromJson<RefCountedPtr<Config>>(config_json, args, errors);
}

RefCountedPtr<grpc_tls_certificate_provider>
FileWatcherCertificateProviderFactory::CreateCertificateProvider(
    RefCountedPtr<CertificateProviderFactory::Config> config) {
  if (config->name() != name()) {
    LOG(ERROR) << "Wrong config type Actual:" << config->name()
               << " vs Expected:" << name();
    return nullptr;
  }
  auto* file_watcher_config =
      DownCast<FileWatcherCertificateProviderFactory::Config*>(config.get());
  return MakeRefCounted<FileWatcherCertificateProvider>(
      file_watcher_config->private_key_file(),
      file_watcher_config->identity_cert_file(),
      file_watcher_config->root_cert_file(),
      file_watcher_config->spiffe_bundle_map_file(),
      file_watcher_config->refresh_interval().millis() / GPR_MS_PER_SEC);
}

void RegisterFileWatcherCertificateProvider(
    CoreConfiguration::Builder* builder) {
  builder->certificate_provider_registry()->RegisterCertificateProviderFactory(
      std::make_unique<FileWatcherCertificateProviderFactory>());
}

}  // namespace grpc_core
