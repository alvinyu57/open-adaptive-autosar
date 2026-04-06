#include <gtest/gtest.h>

#include "ara/exec/exec_error_domain.h"
#include "ara/exec/execution_client.h"

TEST(ExecErrorDomainTest, ReturnsExpectedDomainMetadata) {
    const auto& domain = ara::exec::GetExecErrorDomain();

    EXPECT_STREQ(domain.Name(), "Exec");
    EXPECT_STREQ(domain.Message(static_cast<ara::core::ErrorDomain::CodeType>(
                     ara::exec::ExecErrc::kInvalidTransition)),
                 "invalid transition");
}

TEST(ExecutionClientTest, RejectsMissingTerminationHandler) {
    auto client = ara::exec::ExecutionClient::Create({});

    ASSERT_FALSE(client.HasValue());
    EXPECT_EQ(client.Error(), ara::exec::MakeErrorCode(ara::exec::ExecErrc::kInvalidArgument));
}
