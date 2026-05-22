#include "gtest/gtest.h"
#include <iostream>
#include <cstring>
#include <string>
#include "logging_utils.h"

using namespace logging;

class loggingTest : public ::testing::Test {

    protected:

    loggingTest() {} 

    ~loggingTest() override {}

    void SetUp() override;

    void TearDown() override;

};

void loggingTest::SetUp() {
}

void loggingTest::TearDown() {
}

//Test logging debug function
TEST_F(loggingTest, TestDebug)
{
    const std::string test_str = "Debugging output messages.\n";

    testing::internal::CaptureStderr();
    debug(test_str.c_str());

    const std::string cerr_output = testing::internal::GetCapturedStderr();
    const std::string str = "DEBUG: Debugging output messages.\n";

    #ifndef NGEN_QUIET
        EXPECT_EQ(cerr_output, str);
    #else
        EXPECT_EQ(cerr_output, "");
    #endif
}

//Test logging info function
TEST_F(loggingTest, TestInfo)
{
    const std::string test_str = "Runtime information messages.\n";

    testing::internal::CaptureStderr();
    info(test_str.c_str());

    const std::string cerr_output = testing::internal::GetCapturedStderr();
    const std::string str = "INFO: Runtime information messages.\n";

    #ifndef NGEN_QUIET
        EXPECT_EQ(cerr_output, str);
    #else
        EXPECT_EQ(cerr_output, "");
    #endif
}

//Test logging warning function
TEST_F(loggingTest, TestWarning)
{
    const std::string test_str = "Runtime warning messages.\n";

    testing::internal::CaptureStderr();
    warning(test_str.c_str());

    const std::string cerr_output = testing::internal::GetCapturedStderr();
    const std::string str = "WARNING: Runtime warning messages.\n";

    #ifndef NGEN_QUIET
        EXPECT_EQ(cerr_output, str);
    #else
        EXPECT_EQ(cerr_output, "");
    #endif
}

//Test logging critical function
TEST_F(loggingTest, TestCritical)
{
    const std::string test_str = "Critical runtime messages.\n";

    testing::internal::CaptureStderr();
    critical(test_str.c_str());

    const std::string cerr_output = testing::internal::GetCapturedStderr();
    const std::string str = "CRITICAL: Critical runtime messages.\n";

    EXPECT_EQ(cerr_output, str);
}

//Test logging error function
TEST_F(loggingTest, TestError)
{
    const std::string test_str = "An error has occurred during runtime.\n";

    testing::internal::CaptureStderr();
    error(test_str.c_str());

    const std::string cerr_output = testing::internal::GetCapturedStderr();
    const std::string str = "ERROR: An error has occurred during runtime.\n";

    EXPECT_EQ(cerr_output, str);
}
