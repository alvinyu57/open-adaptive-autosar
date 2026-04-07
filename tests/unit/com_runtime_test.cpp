#include <gtest/gtest.h>

#include "ara/com/runtime.h"

TEST(ComRuntimeTest, ResolvesRegisteredInstanceIdentifiers) {
    auto instance_specifier = ara::core::InstanceSpecifier::Create("/Company/Radar/RequiredPort").Value();
    auto instance_identifier = ara::com::InstanceIdentifier::Create("radar/front-left");

    auto register_result = ara::com::runtime::internal::RegisterInstanceMapping(
        instance_specifier,
        instance_identifier,
        {ara::com::runtime::internal::BindingType::kIpc, ara::core::String("ipc://radar/front-left")});

    ASSERT_TRUE(register_result.HasValue());

    auto resolve_result = ara::com::runtime::ResolveInstanceIDs(instance_specifier);

    ASSERT_TRUE(resolve_result.HasValue());
    ASSERT_EQ(resolve_result.Value().size(), 1U);
    EXPECT_EQ(resolve_result.Value().front(), instance_identifier);
}

TEST(ComRuntimeTest, ReturnsComErrorForUnknownInstanceSpecifier) {
    auto instance_specifier = ara::core::InstanceSpecifier::Create("/Company/Unknown/Port").Value();

    auto resolve_result = ara::com::runtime::ResolveInstanceIDs(instance_specifier);

    ASSERT_FALSE(resolve_result.HasValue());
    EXPECT_EQ(resolve_result.Error(), ara::com::MakeErrorCode(ara::com::ComErrc::kInstanceIDNotResolvable));
}

TEST(ComRuntimeTest, OffersAndFindsIpcServiceBindings) {
    auto instance_identifier = ara::com::InstanceIdentifier::Create("radar/front-center");
    ara::com::ServiceIdentifierType service_id("ara::com.demo.RadarService");

    auto offer_result = ara::com::runtime::internal::OfferService(
        service_id,
        instance_identifier,
        {ara::com::runtime::internal::BindingType::kIpc, ara::core::String("ipc://radar/front-center")});

    ASSERT_TRUE(offer_result.HasValue());

    auto find_result = ara::com::runtime::internal::FindServices(
        service_id,
        instance_identifier);

    ASSERT_TRUE(find_result.HasValue());
    ASSERT_EQ(find_result.Value().size(), 1U);
    EXPECT_EQ(find_result.Value().front().binding_type, ara::com::runtime::internal::BindingType::kIpc);
    EXPECT_EQ(find_result.Value().front().endpoint.View(), "ipc://radar/front-center");

    auto stop_result = ara::com::runtime::internal::StopOfferService(
        service_id,
        instance_identifier);

    EXPECT_TRUE(stop_result.HasValue());
}
