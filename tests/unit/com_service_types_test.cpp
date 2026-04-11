#include <gtest/gtest.h>

#include "ara/com/service/sample_ptr.h"
#include "ara/com/service/service_identifier.h"
#include "ara/com/service/service_version.h"

TEST(ComServiceTypesTest, ServiceIdentifierToStringReturnsOriginalValue) {
    ara::com::ServiceIdentifierType service_identifier("service.alpha");

    EXPECT_EQ(service_identifier.toString(), "service.alpha");
}

TEST(ComServiceTypesTest, ServiceVersionToStringReturnsOriginalValue) {
    ara::com::ServiceVersionType service_version("1.0");

    EXPECT_EQ(service_version.ToString(), "1.0");
}

TEST(ComServiceTypesTest, SamplePtrBehavesLikeUniqueOwnershipPointer) {
    ara::com::SamplePtr<int> sample_ptr(new int(7));

    ASSERT_TRUE(static_cast<bool>(sample_ptr));
    EXPECT_EQ(*sample_ptr, 7);

    sample_ptr.Reset(nullptr);
    EXPECT_FALSE(static_cast<bool>(sample_ptr));
}
