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

#include <grpc/status.h>

#include <memory>

#include "src/core/lib/slice/slice.h"
#include "src/core/util/time.h"
#include "test/core/end2end/end2end_tests.h"

namespace grpc_core {
// Client pings and server pongs. Repeat messages rounds before finishing.
void PingPongStreaming(CoreEnd2endTest& test, int num_messages) {
  auto request_slice = RandomSlice(20);
  auto response_slice = RandomSlice(15);
  auto c = test.NewClientCall("/foo").Timeout(Duration::Seconds(5)).Create();
  IncomingMetadata server_initial_md;
  IncomingStatusOnClient server_status;
  c.NewBatch(1)
      .SendInitialMetadata({})
      .RecvInitialMetadata(server_initial_md)
      .RecvStatusOnClient(server_status);
  auto s = test.RequestCall(100);
  test.Expect(100, true);
  test.Step();
  IncomingCloseOnServer client_close;
  s.NewBatch(101).SendInitialMetadata({}).RecvCloseOnServer(client_close);
  for (int i = 0; i < num_messages; i++) {
    IncomingMessage server_message;
    c.NewBatch(2).SendMessage(request_slice.Ref()).RecvMessage(server_message);
    IncomingMessage client_message;
    s.NewBatch(102).RecvMessage(client_message);
    test.Expect(102, true);
    test.Step();
    s.NewBatch(103).SendMessage(response_slice.Ref());
    test.Expect(2, true);
    test.Expect(103, true);
    test.Step();
  }
  c.NewBatch(3).SendCloseFromClient();
  s.NewBatch(104).SendStatusFromServer(GRPC_STATUS_UNIMPLEMENTED, "xyz", {});
  test.Expect(1, true);
  test.Expect(3, true);
  test.Expect(101, true);
  test.Expect(104, true);
  test.Step();
}

CORE_END2END_TEST(CoreEnd2endTests, PingPongStreaming1) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  PingPongStreaming(*this, 1);
}

CORE_END2END_TEST(CoreEnd2endTests, PingPongStreaming3) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  PingPongStreaming(*this, 3);
}

CORE_END2END_TEST(CoreEnd2endTests, PingPongStreaming10) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  PingPongStreaming(*this, 10);
}

CORE_END2END_TEST(CoreEnd2endTests, PingPongStreaming30) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  PingPongStreaming(*this, 30);
}
}  // namespace grpc_core
