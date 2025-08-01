//
// Copyright 2017 gRPC authors.
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

#ifndef GRPC_SRC_CORE_EXT_TRANSPORT_CHTTP2_TRANSPORT_HTTP2_SETTINGS_MANAGER_H
#define GRPC_SRC_CORE_EXT_TRANSPORT_CHTTP2_TRANSPORT_HTTP2_SETTINGS_MANAGER_H

#include <grpc/support/port_platform.h>
#include <stdint.h>

#include <cstdint>
#include <optional>

#include "absl/functional/function_ref.h"
#include "absl/strings/string_view.h"
#include "src/core/channelz/property_list.h"
#include "src/core/ext/transport/chttp2/transport/frame.h"
#include "src/core/ext/transport/chttp2/transport/http2_settings.h"
#include "src/core/ext/transport/chttp2/transport/http2_status.h"
#include "src/core/util/useful.h"

namespace grpc_core {

class Http2SettingsManager {
 public:
  // Only local and peer settings can be edited by the transport.
  Http2Settings& mutable_local() { return local_; }
  Http2Settings& mutable_peer() { return peer_; }

  const Http2Settings& local() const { return local_; }
  // Before the first SETTINGS ACK frame is received acked_ will hold the
  // default values.
  const Http2Settings& acked() const { return acked_; }
  const Http2Settings& peer() const { return peer_; }

  channelz::PropertyGrid ChannelzProperties() const {
    return channelz::PropertyGrid()
        .SetColumn("local", local_.ChannelzProperties())
        .SetColumn("sent", sent_.ChannelzProperties())
        .SetColumn("peer", peer_.ChannelzProperties())
        .SetColumn("acked", acked_.ChannelzProperties());
  }

  // Returns nullopt if we don't need to send a SETTINGS frame to the peer.
  // Returns Http2SettingsFrame if we need to send a SETTINGS frame to the
  // peer. Transport MUST send a frame returned by this function to the peer.
  // This function is not idempotent.
  std::optional<Http2SettingsFrame> MaybeSendUpdate();

  // Call when we receive an ACK from our peer.
  // This function is not idempotent.
  GRPC_MUST_USE_RESULT bool AckLastSend();

 private:
  enum class UpdateState : uint8_t {
    kFirst,
    kSending,
    kIdle,
  };
  UpdateState update_state_ = UpdateState::kFirst;

  // This holds a copy of the peers settings.
  Http2Settings peer_;

  // These are different sets of our settings.
  // local_  : Setting that has been changed inside our transport,
  //           but not yet sent to the peer.
  // sent_   : New settings frame is sent to the peer but we have not yet
  //           received the ACK from the peer.
  // acked_  : The settings that have already been ACKed by the peer. These
  //           settings can be enforced and any violation of these settings by a
  //           peer may cause an error.
  Http2Settings local_;
  Http2Settings sent_;
  Http2Settings acked_;
};

}  // namespace grpc_core

#endif  // GRPC_SRC_CORE_EXT_TRANSPORT_CHTTP2_TRANSPORT_HTTP2_SETTINGS_MANAGER_H
