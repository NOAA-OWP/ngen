#ifndef NGEN_TEST_SPATIAL_GEOMETRY_LINESTRING_TEST_SUITE_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_LINESTRING_TEST_SUITE_HPP

#include <gtest/gtest.h>
#include <geometry/linestring.hpp>

namespace ngen {
namespace tests {

//! Spatial Features LineString Test Suite
//!
//! Applies the following tests:
//! - OOP-like access to linestring points
//! - OOP-like assignment to linestring points
template<typename DerivedPointType>
void linestring_tests(ngen::spatial::geometry* geom)
{   
    static testing::internal::Random rng{1234};
    static auto ptr = dynamic_cast<ngen::spatial::linestring*>(geom);

    ASSERT_EQ(ptr->type(), ngen::spatial::geometry_t::linestring);

    ptr->resize(10);

    // Reset contents
    for (size_t i = 0; i < ptr->size(); i++) {
        decltype(auto) pt = ptr->get(i);
        pt.x() = rng.Generate(rng.kMaxRange);
        pt.y() = rng.Generate(rng.kMaxRange);
    }

    // Test Case: ensure read access
    double x = 1, y = 2;
    for (size_t i = 0; i < ptr->size(); i++) {
        decltype(auto) pt = ptr->get(i);
        EXPECT_NE(pt.x(), x);
        EXPECT_NE(pt.y(), y);
        pt.x() = x;
        pt.y() = y;
        EXPECT_EQ(pt.x(), x);
        EXPECT_EQ(pt.y(), y);
        x += 2;
        y += 2;
    }

    // Test Case: ensure element assignment
    DerivedPointType new_pt{9, 10};
    ptr->set(1, new_pt);
    EXPECT_EQ(ptr->get(1).x(), 9);
    EXPECT_EQ(ptr->get(1).y(), 10);
};

} // namespace tests
} // namespace ngen

#endif // NGEN_TEST_SPATIAL_GEOMETRY_LINESTRING_TEST_SUITE_HPP
