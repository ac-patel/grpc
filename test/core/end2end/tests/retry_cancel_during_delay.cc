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

#include <grpc/impl/channel_arg_names.h>
#include <grpc/status.h>

#include <memory>
#include <optional>

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/util/time.h"
#include "test/core/end2end/end2end_tests.h"
#include "test/core/end2end/tests/cancel_test_helpers.h"
#include "test/core/test_util/test_config.h"

namespace grpc_core {
namespace {

void TestRetryCancelDuringDelay(
    CoreEnd2endTest& test,
    std::unique_ptr<CancellationMode> cancellation_mode) {
  test.InitServer(CoreEnd2endTest::DefaultServerArgs());
  test.InitClient(ChannelArgs().Set(
      GRPC_ARG_SERVICE_CONFIG,
      absl::StrFormat(
          "{\n"
          "  \"methodConfig\": [ {\n"
          "    \"name\": [\n"
          "      { \"service\": \"service\", \"method\": \"method\" }\n"
          "    ],\n"
          "    \"retryPolicy\": {\n"
          "      \"maxAttempts\": 3,\n"
          "      \"initialBackoff\": \"%ds\",\n"
          "      \"maxBackoff\": \"1000s\",\n"
          "      \"backoffMultiplier\": 1.6,\n"
          "      \"retryableStatusCodes\": [ \"ABORTED\" ]\n"
          "    }\n"
          "  } ]\n"
          "}",
          35 * grpc_test_slowdown_factor())));
  auto expect_finish_before = test.TimestampAfterDuration(Duration::Minutes(2));
  auto c = test.NewClientCall("/service/method")
               .Timeout(Duration::Seconds(20))
               .Create();
  EXPECT_NE(c.GetPeer(), std::nullopt);
  // Client starts a batch with all 6 ops.
  IncomingMetadata server_initial_metadata;
  IncomingMessage server_message;
  IncomingStatusOnClient server_status;
  c.NewBatch(1)
      .SendInitialMetadata({})
      .SendMessage("foo")
      .RecvMessage(server_message)
      .RecvInitialMetadata(server_initial_metadata)
      .RecvStatusOnClient(server_status);
  // Server gets a call and fails with retryable status.
  auto s = test.RequestCall(101);
  test.Expect(101, true);
  test.Step();
  EXPECT_NE(s.GetPeer(), std::nullopt);
  EXPECT_NE(c.GetPeer(), std::nullopt);
  IncomingCloseOnServer client_close;
  s.NewBatch(102)
      .SendInitialMetadata({})
      .SendStatusFromServer(GRPC_STATUS_ABORTED, "xyz", {})
      .RecvCloseOnServer(client_close);
  test.Expect(102, true);
  test.Step();
  // Server should never get a second call, because the initial retry
  // delay is longer than the call's deadline.
  auto s2 = test.RequestCall(201);
  // Initiate cancellation.
  cancellation_mode->Apply(c);
  test.Expect(1, true);
  test.Step(Duration::Minutes(1));
  auto finish_time = Timestamp::Now();
  EXPECT_EQ(server_status.status(), cancellation_mode->ExpectedStatus())
      << server_status.message();
  EXPECT_FALSE(client_close.was_cancelled());
  // Make sure we didn't wait the full deadline before failing.
  EXPECT_LT(finish_time, expect_finish_before);
  // Shutdown the server to gc the requested call.
  test.ShutdownServerAndNotify(1000);
  test.Expect(1000, true);
  test.Expect(201, false);
  test.Step();
}

CORE_END2END_TEST(RetryTests, CancelDuringDelay) {
  if (!IsRetryInCallv3Enabled()) SKIP_IF_V3();
  TestRetryCancelDuringDelay(*this, std::make_unique<CancelCancellationMode>());
}

CORE_END2END_TEST(RetryTests, DeadlineDuringDelay) {
  SKIP_IF_V3();  // Not working yet
  TestRetryCancelDuringDelay(*this,
                             std::make_unique<DeadlineCancellationMode>());
}

}  // namespace
}  // namespace grpc_core
