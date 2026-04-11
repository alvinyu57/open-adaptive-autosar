#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <unistd.h>

#include "ara/com/runtime.h"

namespace {

std::string UniqueName(const std::string& prefix) {
    static std::atomic<unsigned long long> counter{0U};
    return prefix + "_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
}

} // namespace

TEST(ComRuntimeTest, ResolvesRegisteredInstanceIdentifiers) {
    auto instance_specifier =
        ara::core::InstanceSpecifier::Create("/Company/Radar/RequiredPort").Value();
    auto instance_identifier = ara::com::InstanceIdentifier::Create("radar/front-left");

    auto register_result = ara::com::runtime::internal::RegisterInstanceMapping(
        instance_specifier,
        instance_identifier,
        {ara::com::runtime::internal::BindingType::kIpc,
         ara::core::String("ipc://radar/front-left")});

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
    EXPECT_EQ(resolve_result.Error(),
              ara::com::MakeErrorCode(ara::com::ComErrc::kInstanceIDNotResolvable));
}

TEST(ComRuntimeTest, OffersAndFindsIpcServiceBindings) {
    auto instance_identifier = ara::com::InstanceIdentifier::Create("radar/front-center");
    ara::com::ServiceIdentifierType service_id("ara::com.demo.RadarService");

    auto offer_result =
        ara::com::runtime::internal::OfferService(service_id,
                                                  instance_identifier,
                                                  {ara::com::runtime::internal::BindingType::kIpc,
                                                   ara::core::String("ipc://radar/front-center")});

    ASSERT_TRUE(offer_result.HasValue());

    auto find_result = ara::com::runtime::internal::FindServices(service_id, instance_identifier);

    ASSERT_TRUE(find_result.HasValue());
    ASSERT_EQ(find_result.Value().size(), 1U);
    EXPECT_EQ(find_result.Value().front().binding_type,
              ara::com::runtime::internal::BindingType::kIpc);
    EXPECT_EQ(find_result.Value().front().endpoint.View(), "ipc://radar/front-center");

    auto stop_result =
        ara::com::runtime::internal::StopOfferService(service_id, instance_identifier);

    EXPECT_TRUE(stop_result.HasValue());
}

TEST(ComRuntimeTest, PublishesAndReadsSharedMemoryEvents) {
    const auto channel_name = UniqueName("ara_com_event");
    std::uint64_t last_seen_sequence{0U};

    auto publish_result =
        ara::com::runtime::internal::PublishEvent(channel_name, R"({"pressureKPa":99})");
    ASSERT_TRUE(publish_result.HasValue());

    auto first_read_result =
        ara::com::runtime::internal::GetNewEvent(channel_name, last_seen_sequence);
    ASSERT_TRUE(first_read_result.HasValue());
    ASSERT_TRUE(first_read_result.Value().has_value());
    EXPECT_EQ(first_read_result.Value()->View(), R"({"pressureKPa":99})");

    auto second_read_result =
        ara::com::runtime::internal::GetNewEvent(channel_name, last_seen_sequence);
    ASSERT_TRUE(second_read_result.HasValue());
    EXPECT_FALSE(second_read_result.Value().has_value());
}

TEST(ComRuntimeTest, CompletesSharedMemoryRequestResponseRoundTrip) {
    const auto channel_name = UniqueName("ara_com_method");
    std::uint64_t last_seen_sequence{0U};

    auto future = std::async(std::launch::async, [&channel_name]() {
        return ara::com::runtime::internal::CallMethod(
            channel_name,
            "GetLatestPressure",
            std::chrono::milliseconds(1000));
    });

    std::optional<ara::com::runtime::internal::ChannelMessage> request;
    for (int attempt = 0; attempt < 20 && !request.has_value(); ++attempt) {
        auto take_result =
            ara::com::runtime::internal::TakeMethodCall(channel_name, last_seen_sequence);
        ASSERT_TRUE(take_result.HasValue());
        request = take_result.Value();
        if (!request.has_value()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->payload.View(), "GetLatestPressure");

    auto response_result = ara::com::runtime::internal::SendMethodResponse(
        channel_name,
        request->correlation_id,
        R"({"pressureKPa":101})");
    ASSERT_TRUE(response_result.HasValue());

    auto call_result = future.get();
    ASSERT_TRUE(call_result.HasValue());
    EXPECT_EQ(call_result.Value().View(), R"({"pressureKPa":101})");
}

TEST(ComRuntimeTest, SendsAndReceivesSharedMemoryFireAndForgetMessages) {
    const auto channel_name = UniqueName("ara_com_oneway");
    std::uint64_t last_seen_sequence{0U};

    auto send_result =
        ara::com::runtime::internal::SendFireAndForget(channel_name, "LowPressureAlarm:front_left=98");
    ASSERT_TRUE(send_result.HasValue());

    auto take_result =
        ara::com::runtime::internal::TakeFireAndForget(channel_name, last_seen_sequence);
    ASSERT_TRUE(take_result.HasValue());
    ASSERT_TRUE(take_result.Value().has_value());
    EXPECT_EQ(take_result.Value()->View(), "LowPressureAlarm:front_left=98");

    auto second_take_result =
        ara::com::runtime::internal::TakeFireAndForget(channel_name, last_seen_sequence);
    ASSERT_TRUE(second_take_result.HasValue());
    EXPECT_FALSE(second_take_result.Value().has_value());
}
