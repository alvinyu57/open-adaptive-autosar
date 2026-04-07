#include <gtest/gtest.h>

#include "ara/com/com_error_domain.h"

TEST(ComErrorDomainTest, ReturnsExpectedDomainName) {
    const auto& domain = ara::com::GetComErrorDomain();
    EXPECT_STREQ(domain.Name(), "Com");
}

TEST(ComErrorDomainTest, ReturnsCorrectMessageForInstanceNotResolvable) {
    const auto& domain = ara::com::GetComErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::com::ComErrc::kInstanceIDNotResolvable));
    EXPECT_STREQ(message, "instance identifier not resolvable");
}

TEST(ComErrorDomainTest, ThrowsComExceptionWithValidErrorCode) {
    const auto& domain = ara::com::GetComErrorDomain();
    auto error_code = ara::com::MakeErrorCode(ara::com::ComErrc::kCommunicationFailure);

    EXPECT_THROW(domain.ThrowAsException(error_code), ara::com::ComException);
}
