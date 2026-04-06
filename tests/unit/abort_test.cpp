#include <gtest/gtest.h>

#include <string_view>

#include "ara/core/abort.h"

class AbortTest : public ::testing::Test {
protected:
    static int handler_call_count;

    static void TestAbortHandler1(std::string_view message) noexcept {
        (void)message; // suppress unused parameter warning
        handler_call_count++;
    }

    static void TestAbortHandler2(std::string_view message) noexcept {
        (void)message; // suppress unused parameter warning
        handler_call_count++;
    }

    void SetUp() override {
        handler_call_count = 0;
    }
};

int AbortTest::handler_call_count = 0;

TEST_F(AbortTest, AddAbortHandlerReturnsTrue) {
    EXPECT_TRUE(ara::core::AddAbortHandler(TestAbortHandler1));
}

TEST_F(AbortTest, AddAbortHandlerReturnsFalseForNullptr) {
    EXPECT_FALSE(ara::core::AddAbortHandler(nullptr));
}

TEST_F(AbortTest, AddAbortHandlerReturnsFalseForDuplicateHandler) {
    // First call should succeed (or may already be registered from previous tests)
    ara::core::AddAbortHandler(TestAbortHandler2);

    // Second call with same handler should fail
    EXPECT_FALSE(ara::core::AddAbortHandler(TestAbortHandler2));
}

TEST_F(AbortTest, CanAddMultipleDifferentHandlers) {
    static int handler1_calls = 0;
    static int handler2_calls = 0;

    auto handler1 = [](std::string_view) noexcept {
        handler1_calls++;
    };
    auto handler2 = [](std::string_view) noexcept {
        handler2_calls++;
    };

    EXPECT_TRUE(ara::core::AddAbortHandler(handler1));
    EXPECT_TRUE(ara::core::AddAbortHandler(handler2));
}
