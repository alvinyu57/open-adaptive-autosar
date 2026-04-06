#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "ara/log/logger.hpp"

class LoggerTest : public ::testing::Test {
protected:
    std::ostringstream output_stream;
    ara::log::Logger logger{output_stream};

    bool OutputContains(const std::string& text) const {
        return output_stream.str().find(text) != std::string::npos;
    }
};

TEST_F(LoggerTest, InfoLogsWithCorrectLevel) {
    logger.Info("TestComponent", "Test info message");
    
    EXPECT_TRUE(OutputContains("[INFO]"));
    EXPECT_TRUE(OutputContains("TestComponent"));
    EXPECT_TRUE(OutputContains("Test info message"));
}

TEST_F(LoggerTest, WarnLogsWithCorrectLevel) {
    logger.Warn("TestComponent", "Test warning message");
    
    EXPECT_TRUE(OutputContains("[WARN]"));
    EXPECT_TRUE(OutputContains("TestComponent"));
    EXPECT_TRUE(OutputContains("Test warning message"));
}

TEST_F(LoggerTest, ErrorLogsWithCorrectLevel) {
    logger.Error("TestComponent", "Test error message");
    
    EXPECT_TRUE(OutputContains("[ERROR]"));
    EXPECT_TRUE(OutputContains("TestComponent"));
    EXPECT_TRUE(OutputContains("Test error message"));
}

TEST_F(LoggerTest, InfoLogsIncludeTimestamp) {
    logger.Info("Component", "Message");
    
    const auto output = output_stream.str();
    // Timestamp format: YYYY-MM-DD HH:MM:SS.mmm
    // Check for date pattern (at least contain hyphens and colons)
    EXPECT_TRUE(output.find("-") != std::string::npos);
    EXPECT_TRUE(output.find(":") != std::string::npos);
}

TEST_F(LoggerTest, MultipleLogsAreWritten) {
    logger.Info("Comp1", "Message 1");
    logger.Warn("Comp2", "Message 2");
    logger.Error("Comp3", "Message 3");
    
    const auto output = output_stream.str();
    EXPECT_TRUE(output.find("Message 1") != std::string::npos);
    EXPECT_TRUE(output.find("Message 2") != std::string::npos);
    EXPECT_TRUE(output.find("Message 3") != std::string::npos);
}

TEST_F(LoggerTest, EmptyComponentNameIsAllowed) {
    logger.Info("", "Message");
    
    EXPECT_TRUE(OutputContains("[INFO]"));
    EXPECT_TRUE(OutputContains("Message"));
}

TEST_F(LoggerTest, EmptyMessageIsAllowed) {
    logger.Warn("Component", "");
    
    EXPECT_TRUE(OutputContains("[WARN]"));
    EXPECT_TRUE(OutputContains("Component"));
}
