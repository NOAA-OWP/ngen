#include "gtest/gtest.h"

#include <simulation_time/simulation_clock.hpp>

using clock_type = ngen::simulation_clock;

// 1704067200 -> 01-01-2024 00:00:00 UTC
constexpr std::time_t epoch_unix = 1704067200L;
const auto epoch_calendar = clock_type::calendar_clock::from_time_t(epoch_unix);

TEST(SimulationClockTest, TestConfiguration)
{
    ASSERT_FALSE(clock_type::has_epoch());
    ASSERT_NO_THROW(clock_type::set_epoch(epoch_calendar));
    ASSERT_TRUE(clock_type::has_epoch());
    ASSERT_THROW(clock_type::set_epoch(epoch_calendar), std::logic_error);
    EXPECT_EQ(clock_type::epoch(), epoch_calendar);
}

TEST(SimulationClockTest, TestConversion)
{
    const auto now = clock_type::now();
    EXPECT_EQ(clock_type::from_time_t(epoch_unix), now);
    EXPECT_EQ(clock_type::from_calendar(epoch_calendar), now);
    EXPECT_EQ(now, clock_type::time_point{});
    EXPECT_EQ(clock_type::to_time_t(now), epoch_unix);
    EXPECT_EQ(clock_type::to_calendar(now), epoch_calendar);
}

TEST(SimulationClockTest, TestTicking)
{
    const auto now = clock_type::now();
    auto forward = std::chrono::seconds{1};
    ASSERT_THROW(clock_type::tick(-1), std::domain_error);
    ASSERT_NO_THROW(clock_type::tick(1));
    EXPECT_EQ(clock_type::now(), now + forward);

    forward += std::chrono::minutes{5};
    ASSERT_THROW(clock_type::tick(std::chrono::minutes{-5}), std::domain_error);
    ASSERT_NO_THROW(clock_type::tick(std::chrono::minutes{5}));
    EXPECT_EQ(clock_type::now(), now + forward);

    EXPECT_EQ(
        clock_type::to_calendar(clock_type::now()),
        epoch_calendar + forward
    );
}
