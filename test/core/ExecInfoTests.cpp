#include <gtest/gtest.h>

#include <NGenConfig.h>

#include <sstream>

//! runtime_usage() should emit non-empty usage text to the provided stream.
TEST(ExecInfo_Test, RuntimeUsageEmits)
{
    std::ostringstream out;
    ngen::exec_info::runtime_usage("ngen", out);
    EXPECT_FALSE(out.str().empty());
}

//! runtime_summary() should emit to the provided stream without throwing.
TEST(ExecInfo_Test, RuntimeSummaryEmits)
{
    std::ostringstream out;
    ngen::exec_info::runtime_summary(out);
    EXPECT_FALSE(out.str().empty());
}
