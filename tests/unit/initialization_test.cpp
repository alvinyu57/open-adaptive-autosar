#include <gtest/gtest.h>

#include "ara/core/initialization.h"
#include "ara/core/core_error_domain.h"

class InitializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Deinitialize before each test to ensure clean state
        ara::core::Deinitialize();
    }

    void TearDown() override {
        // Clean up after each test
        ara::core::Deinitialize();
    }
};

TEST_F(InitializationTest, InitializeSucceedsWhenNotInitialized) {
    const auto result = ara::core::Initialize();
    EXPECT_TRUE(result.HasValue());
}

TEST_F(InitializationTest, InitializeFailsWhenAlreadyInitialized) {
    // First initialization should succeed
    auto result1 = ara::core::Initialize();
    EXPECT_TRUE(result1.HasValue());

    // Second initialization should fail
    auto result2 = ara::core::Initialize();
    EXPECT_FALSE(result2.HasValue());
    EXPECT_EQ(result2.Error(), ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState));
}

TEST_F(InitializationTest, InitializeWithArgsSucceedsWhenNotInitialized) {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};

    const auto result = ara::core::Initialize(argc, argv);
    EXPECT_TRUE(result.HasValue());
}

TEST_F(InitializationTest, InitializeWithArgsFailsWhenAlreadyInitialized) {
    // First initialization should succeed
    auto result1 = ara::core::Initialize();
    EXPECT_TRUE(result1.HasValue());

    // Second initialization with args should fail
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    auto result2 = ara::core::Initialize(argc, argv);
    EXPECT_FALSE(result2.HasValue());
    EXPECT_EQ(result2.Error(), ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState));
}

TEST_F(InitializationTest, DeinitializeSucceedsWhenInitialized) {
    // Initialize first
    ara::core::Initialize();

    // Then deinitialize
    const auto result = ara::core::Deinitialize();
    EXPECT_TRUE(result.HasValue());
}

TEST_F(InitializationTest, DeinitializeFailsWhenNotInitialized) {
    const auto result = ara::core::Deinitialize();
    EXPECT_FALSE(result.HasValue());
    EXPECT_EQ(result.Error(), ara::core::MakeErrorCode(ara::core::CoreErrc::kInvalidState));
}

TEST_F(InitializationTest, CanReinitializeAfterDeinitialize) {
    // Initialize -> Deinitialize -> Initialize again
    auto init1 = ara::core::Initialize();
    EXPECT_TRUE(init1.HasValue());

    auto deinit = ara::core::Deinitialize();
    EXPECT_TRUE(deinit.HasValue());

    auto init2 = ara::core::Initialize();
    EXPECT_TRUE(init2.HasValue());
}
