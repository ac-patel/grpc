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

#include <grpc/status.h>

#include <memory>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "gtest/gtest.h"
#include "src/core/util/time.h"
#include "test/core/end2end/cq_verifier.h"
#include "test/core/end2end/end2end_tests.h"
#include "test/core/end2end/fixtures/h2_tls_common.h"

namespace grpc_core {
namespace {

// Client requests status along with the initial metadata. Server streams
// messages and ends with a non-OK status. Client reads after server is done
// writing, and expects to get the status after the messages.
void ServerStreaming(CoreEnd2endTest& test, int num_messages) {
  auto c = test.NewClientCall("/foo").Timeout(Duration::Minutes(1)).Create();
  IncomingMetadata server_initial_metadata;
  IncomingStatusOnClient server_status;
  c.NewBatch(1)
      .SendInitialMetadata({})
      .RecvInitialMetadata(server_initial_metadata)
      // Client requests status early but should not receive status till all the
      // messages are received.
      .RecvStatusOnClient(server_status);
  // Client sends close early
  c.NewBatch(3).SendCloseFromClient();
  test.Expect(3, true);
  test.Step();
  auto s = test.RequestCall(100);
  test.Expect(100, true);
  test.Step();
  s.NewBatch(101).SendInitialMetadata({});
  test.Expect(101, true);
  test.Step();
  // Server writes bunch of messages
  for (int i = 0; i < num_messages; i++) {
    s.NewBatch(103).SendMessage("hello world");
    test.Expect(103, true);
    test.Step();
  }
  // Server sends status
  IncomingCloseOnServer client_close;
  s.NewBatch(104)
      .SendStatusFromServer(GRPC_STATUS_UNIMPLEMENTED, "xyz", {})
      .RecvCloseOnServer(client_close);
  bool seen_status = false;
  test.Expect(1, CoreEnd2endTest::Maybe{&seen_status});
  test.Expect(104, true);
  test.Step();

  VLOG(2) << "SEEN_STATUS:" << seen_status;

  // Client keeps reading messages till it gets the status
  int num_messages_received = 0;
  while (true) {
    IncomingMessage server_message;
    c.NewBatch(102).RecvMessage(server_message);
    test.Expect(1, CqVerifier::Maybe{&seen_status});
    test.Expect(102, true);
    test.Step();
    if (server_message.is_end_of_stream()) {
      // The transport has received the trailing metadata.
      break;
    }
    EXPECT_EQ(server_message.payload(), "hello world");
    num_messages_received++;
  }
  CHECK_EQ(num_messages_received, num_messages);
  if (!seen_status) {
    test.Expect(1, true);
    test.Step();
  }
  EXPECT_EQ(server_status.status(), GRPC_STATUS_UNIMPLEMENTED);
  EXPECT_EQ(server_status.message(), "xyz");
}

CORE_END2END_TEST(Http2Tests, ServerStreaming) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  ServerStreaming(*this, 1);
}

CORE_END2END_TEST(Http2Tests, ServerStreamingEmptyStream) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  ServerStreaming(*this, 0);
}

CORE_END2END_TEST(Http2Tests, ServerStreaming10Messages) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  // TODO(yashykt): Remove this once b/376283636 is fixed.
  ConfigVars::Overrides overrides;
  overrides.default_ssl_roots_file_path = CA_CERT_PATH;
  overrides.trace =
      "call,channel,client_channel,client_channel_call,client_channel_lb_call,"
      "http";
  ConfigVars::SetOverrides(overrides);
  ServerStreaming(*this, 10);
}

}  // namespace
}  // namespace grpc_core
