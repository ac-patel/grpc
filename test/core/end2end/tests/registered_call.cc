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

#include "gtest/gtest.h"
#include "src/core/util/time.h"
#include "test/core/end2end/end2end_tests.h"

namespace grpc_core {
namespace {

void SimpleRequestBody(CoreEnd2endTest& test,
                       CoreEnd2endTest::RegisteredCall rc) {
  auto c = test.NewClientCall(rc).Timeout(Duration::Seconds(5)).Create();
  IncomingStatusOnClient server_status;
  IncomingMetadata server_initial_metadata;
  c.NewBatch(1)
      .SendInitialMetadata({})
      .SendCloseFromClient()
      .RecvInitialMetadata(server_initial_metadata)
      .RecvStatusOnClient(server_status);
  auto s = test.RequestCall(101);
  test.Expect(101, true);
  test.Step();
  IncomingCloseOnServer client_close;
  s.NewBatch(102)
      .SendInitialMetadata({})
      .SendStatusFromServer(GRPC_STATUS_UNIMPLEMENTED, "xyz", {})
      .RecvCloseOnServer(client_close);
  test.Expect(102, true);
  test.Expect(1, true);
  test.Step();
  EXPECT_EQ(server_status.status(), GRPC_STATUS_UNIMPLEMENTED);
  EXPECT_EQ(server_status.message(), "xyz");
  EXPECT_EQ(s.method(), "/foo");
  EXPECT_FALSE(client_close.was_cancelled());
}

CORE_END2END_TEST(CoreEnd2endTests, InvokeRegisteredCall) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  SimpleRequestBody(*this, RegisterCallOnClient("/foo", nullptr));
}

CORE_END2END_TEST(CoreEnd2endTests, Invoke10RegisteredCalls) {
  SKIP_TEST_PH2_CLIENT();  // TODO(tjagtap) [PH2][P2] Can test be enabled?
  auto rc = RegisterCallOnClient("/foo", nullptr);
  for (int i = 0; i < 10; i++) {
    SimpleRequestBody(*this, rc);
  }
}

}  // namespace
}  // namespace grpc_core
