#include <gtest/gtest.h>

#include "ara/core/core_error_domain.h"

TEST(CoreErrorDomainTest, ReturnsExpectedDomainName) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    EXPECT_STREQ(domain.Name(), "Core");
}

TEST(CoreErrorDomainTest, ReturnsCorrectMessageForInvalidArgument) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::core::CoreErrc::kInvalidArgument));
    EXPECT_STREQ(message, "invalid argument");
}

TEST(CoreErrorDomainTest, ReturnsCorrectMessageForInvalidState) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::core::CoreErrc::kInvalidState));
    EXPECT_STREQ(message, "invalid state");
}

TEST(CoreErrorDomainTest, ReturnsCorrectMessageForOutOfRange) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::core::CoreErrc::kOutOfRange));
    EXPECT_STREQ(message, "out of range");
}

TEST(CoreErrorDomainTest, ReturnsCorrectMessageForNoSuchElement) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::core::CoreErrc::kNoSuchElement));
    EXPECT_STREQ(message, "no such element");
}

TEST(CoreErrorDomainTest, ReturnsCorrectMessageForAlreadyExists) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(
        static_cast<ara::core::ErrorDomain::CodeType>(ara::core::CoreErrc::kAlreadyExists));
    EXPECT_STREQ(message, "already exists");
}

TEST(CoreErrorDomainTest, ReturnsUnknownMessageForInvalidCode) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    const auto* const message = domain.Message(9999);
    EXPECT_STREQ(message, "unknown core error");
}

TEST(CoreErrorDomainTest, ThrowsCoreExceptionWithValidErrorCode) {
    const auto& domain = ara::core::GetCoreErrorDomain();
    auto error_code = ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidArgument);

    EXPECT_THROW(domain.ThrowAsException(error_code), ara::core::CoreException);
}
