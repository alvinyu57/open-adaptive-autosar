#include <gtest/gtest.h>

#include "ara/core/instance_specifier.h"
#include "ara/exec/function_group.h"
#include "ara/exec/function_group_state.h"
#include "ara/exec/state_client.h"

TEST(StateClientTest, AcceptsFunctionGroupStateTransition) {
    auto client = ara::exec::StateClient::Create([](const ara::exec::ExecutionErrorEvent&) {});
    ASSERT_TRUE(client.HasValue());

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    auto transition = client.Value().SetState(ara::exec::FunctionGroupState(machine_fg, "Startup"));

    EXPECT_NO_THROW(transition.get());
}

TEST(StateClientTest, RejectsMissingUndefinedStateCallback) {
    auto client = ara::exec::StateClient::Create({});

    ASSERT_FALSE(client.HasValue());
    EXPECT_EQ(client.Error(), ara::exec::MakeErrorCode(ara::exec::ExecErrc::kInvalidArgument));
}

TEST(StateClientTest, ReportsNoExecutionErrorForDefinedState) {
    auto client = ara::exec::StateClient::Create([](const ara::exec::ExecutionErrorEvent&) {});
    ASSERT_TRUE(client.HasValue());

    ara::exec::FunctionGroup machine_fg(ara::core::InstanceSpecifier("MachineFG"));
    auto result =
        client.Value().GetExecutionError(ara::exec::FunctionGroupState(machine_fg, "Startup"));

    ASSERT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error(), ara::exec::MakeErrorCode(ara::exec::ExecErrc::kOperationFailed));
}
