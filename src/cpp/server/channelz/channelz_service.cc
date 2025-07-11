//
//
// Copyright 2018 gRPC authors.
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

#include "src/cpp/server/channelz/channelz_service.h"

#include <grpc/support/alloc.h>
#include <grpc/support/port_platform.h>
#include <grpcpp/impl/codegen/config_protobuf.h>

#include <memory>

#include "absl/log/log.h"
#include "src/core/channelz/channelz_registry.h"

using grpc_core::channelz::BaseNode;

// IWYU pragma: no_include "google/protobuf/json/json.h"
// IWYU pragma: no_include "google/protobuf/util/json_util.h"

// IWYU pragma: no_include <google/protobuf/util/json_util.h>

namespace grpc {

namespace {

constexpr size_t kMaxResults = 100;

grpc::protobuf::util::Status ParseJson(const char* json_str,
                                       grpc::protobuf::Message* message) {
  grpc::protobuf::json::JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  auto r =
      grpc::protobuf::json::JsonStringToMessage(json_str, message, options);
  if (!r.ok()) {
    LOG(ERROR) << "channelz json parse failed: error=" << r.ToString()
               << " json:\n"
               << json_str;
  }
  return r;
}

}  // namespace

Status ChannelzService::GetTopChannels(
    ServerContext* /*unused*/,
    const channelz::v1::GetTopChannelsRequest* request,
    channelz::v1::GetTopChannelsResponse* response) {
  char* json_str = grpc_channelz_get_top_channels(request->start_channel_id());
  if (json_str == nullptr) {
    return Status(StatusCode::INTERNAL,
                  "grpc_channelz_get_top_channels returned null");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetServers(
    ServerContext* /*unused*/, const channelz::v1::GetServersRequest* request,
    channelz::v1::GetServersResponse* response) {
  char* json_str = grpc_channelz_get_servers(request->start_server_id());
  if (json_str == nullptr) {
    return Status(StatusCode::INTERNAL,
                  "grpc_channelz_get_servers returned null");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetServer(ServerContext* /*unused*/,
                                  const channelz::v1::GetServerRequest* request,
                                  channelz::v1::GetServerResponse* response) {
  char* json_str = grpc_channelz_get_server(request->server_id());
  if (json_str == nullptr) {
    return Status(StatusCode::INTERNAL,
                  "grpc_channelz_get_server returned null");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetServerSockets(
    ServerContext* /*unused*/,
    const channelz::v1::GetServerSocketsRequest* request,
    channelz::v1::GetServerSocketsResponse* response) {
  char* json_str = grpc_channelz_get_server_sockets(
      request->server_id(), request->start_socket_id(), request->max_results());
  if (json_str == nullptr) {
    return Status(StatusCode::INTERNAL,
                  "grpc_channelz_get_server_sockets returned null");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetChannel(
    ServerContext* /*unused*/, const channelz::v1::GetChannelRequest* request,
    channelz::v1::GetChannelResponse* response) {
  char* json_str = grpc_channelz_get_channel(request->channel_id());
  if (json_str == nullptr) {
    return Status(StatusCode::NOT_FOUND, "No object found for that ChannelId");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetSubchannel(
    ServerContext* /*unused*/,
    const channelz::v1::GetSubchannelRequest* request,
    channelz::v1::GetSubchannelResponse* response) {
  char* json_str = grpc_channelz_get_subchannel(request->subchannel_id());
  if (json_str == nullptr) {
    return Status(StatusCode::NOT_FOUND,
                  "No object found for that SubchannelId");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzService::GetSocket(ServerContext* /*unused*/,
                                  const channelz::v1::GetSocketRequest* request,
                                  channelz::v1::GetSocketResponse* response) {
  char* json_str = grpc_channelz_get_socket(request->socket_id());
  if (json_str == nullptr) {
    return Status(StatusCode::NOT_FOUND, "No object found for that SocketId");
  }
  grpc::protobuf::util::Status s = ParseJson(json_str, response);
  gpr_free(json_str);
  if (!s.ok()) {
    return Status(StatusCode::INTERNAL, s.ToString());
  }
  return Status::OK;
}

Status ChannelzV2Service::QueryEntities(
    ServerContext* /*unused*/,
    const channelz::v2::QueryEntitiesRequest* request,
    channelz::v2::QueryEntitiesResponse* response) {
  std::optional<BaseNode::EntityType> type =
      BaseNode::KindToEntityType(request->kind());
  if (!type.has_value()) {
    return Status(StatusCode::INVALID_ARGUMENT,
                  absl::StrCat("Invalid entity kind: ", request->kind()));
  }
  grpc_core::WeakRefCountedPtr<BaseNode> parent;
  if (request->parent() != 0) {
    parent = grpc_core::channelz::ChannelzRegistry::GetNode(request->parent());
    if (parent == nullptr) {
      return Status(StatusCode::NOT_FOUND,
                    "No object found for parent EntityId");
    }
  }
  const auto [nodes, end] =
      parent != nullptr
          ? grpc_core::channelz::ChannelzRegistry::GetChildrenOfType(
                request->start_entity_id(), parent.get(), *type, kMaxResults)
          : grpc_core::channelz::ChannelzRegistry::GetNodesOfType(
                request->start_entity_id(), *type, kMaxResults);
  response->set_end(end);
  for (const auto& node : nodes) {
    response->add_entities()->ParseFromString(node->SerializeEntityToString());
  }
  return Status::OK;
}

Status ChannelzV2Service::GetEntity(
    ServerContext* /*unused*/, const channelz::v2::GetEntityRequest* request,
    channelz::v2::GetEntityResponse* response) {
  auto node = grpc_core::channelz::ChannelzRegistry::GetNode(request->id());
  if (node == nullptr) {
    return Status(StatusCode::NOT_FOUND, "No object found for that EntityId");
  }
  response->mutable_entity()->ParseFromString(node->SerializeEntityToString());
  return Status::OK;
}

}  // namespace grpc
