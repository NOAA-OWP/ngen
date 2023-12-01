#ifndef NGEN_TEST_SPATIAL_GEOMETRY_POINT_TEST_SUITE_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_POINT_TEST_SUITE_HPP

#include <gtest/gtest.h>
#include <geometry/point.hpp>

namespace ngen {
namespace tests {

//! Spatial Features Point Test Suite
//!
//! Applies the following tests:
//! - Reference access for X coordinate
//! - Reference access for Y coordinate
//! - Reference assignment for X coordinate
//! - Reference assignment for Y coordinate
void point_tests(ngen::spatial::geometry* geom)
{
    static testing::internal::Random rng{1234};
    static auto ptr = dynamic_cast<ngen::spatial::point*>(geom);

    ASSERT_EQ(ptr->type(), ngen::spatial::geometry_t::point);

    ASSERT_NO_FATAL_FAILURE(ptr->x() = rng.Generate(rng.kMaxRange));
    ASSERT_NO_FATAL_FAILURE(ptr->y() = rng.Generate(rng.kMaxRange));

    const double x = 3.584;
    const double y = 34.4128;
    EXPECT_NE(ptr->x(), x);
    EXPECT_NE(ptr->y(), y);

    ptr->x() = x;
    ptr->y() = y;
    EXPECT_NEAR(ptr->x(), x, 1e-6);
    EXPECT_NEAR(ptr->y(), y, 1e-6);
}

} // namespace tests
} // namespace ngen


#endif // NGEN_TEST_SPATIAL_GEOMETRY_POINT_TESTS_HPP
