#ifndef NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP

#include <gtest/gtest.h>

#include <geometry/geometry.hpp>
#include <geometry/point.hpp>
#include <geometry/linestring.hpp>
#include <geometry/polygon.hpp>
#include <initializer_list>
#include "geometry/backends/boost/boost_point.hpp"

#define NGEN_FORCE_INLINE inline __attribute__((always_inline))

#define NGEN_GEOMETRY_TEST(TestName, TypeName, ...)                      \
    TEST(SpatialGeometry, TestName) {                                    \
        TypeName x{__VA_ARGS__};                                         \
        ngen::spatial::tests::perform_geometry_tests(std::addressof(x)); \
    }

namespace ngen {
namespace spatial {
namespace tests {

namespace detail {

//! Spatial Features Point Test Suite
//!
//! Applies the following tests:
//! - Reference access for X coordinate
//! - Reference access for Y coordinate
//! - Reference assignment for X coordinate
//! - Reference assignment for Y coordinate
NGEN_FORCE_INLINE void point_tests(ngen::spatial::geometry* geom)
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

//! Spatial Features LineString Test Suite
//!
//! Applies the following tests:
//! - OOP-like access to linestring points
//! - OOP-like assignment to linestring points
NGEN_FORCE_INLINE void linestring_tests(ngen::spatial::geometry* geom)
{   
    static testing::internal::Random rng{1234};
    static auto ptr = dynamic_cast<ngen::spatial::linestring*>(geom);

    ASSERT_EQ(ptr->type(), ngen::spatial::geometry_t::linestring);

    ptr->resize(10);

    // Reset contents
    for (size_t i = 0; i < ptr->size(); i++) {
        decltype(auto) pt = ptr->get(i);
        pt->x() = rng.Generate(rng.kMaxRange);
        pt->y() = rng.Generate(rng.kMaxRange);
    }

    // Test Case: ensure read access
    double x = 1, y = 2;
    for (size_t i = 0; i < ptr->size(); i++) {
        decltype(auto) pt = ptr->get(i);
        EXPECT_NE(pt->x(), x);
        EXPECT_NE(pt->y(), y);
        pt->x() = x;
        pt->y() = y;
        EXPECT_EQ(pt->x(), x);
        EXPECT_EQ(pt->y(), y);
        x += 2;
        y += 2;
    }

    // Test Case: ensure element assignment
    boost::boost_point new_pt{9, 10};
    ptr->set(1, &new_pt);
    EXPECT_EQ(ptr->get(1)->x(), 9);
    EXPECT_EQ(ptr->get(1)->y(), 10);
};

NGEN_FORCE_INLINE void polygon_tests(ngen::spatial::geometry* geom)
{
    static testing::internal::Random rng{1234};
    static auto ptr = dynamic_cast<ngen::spatial::polygon*>(geom);

    ASSERT_EQ(ptr->type(), ngen::spatial::geometry_t::polygon);

    // TODO:
};

} // namespace detail

//! Apply test suites to an abstract geometry dependent on its derived type.
NGEN_FORCE_INLINE void perform_geometry_tests(ngen::spatial::geometry* geom)
{
    if (geom == nullptr) {
        FAIL() << "No geometry not assigned to this instance of GeometryTest";
    }

    using geometry_type = ngen::spatial::geometry_t;

    geometry_type derived = geom->type();

    std::string typestr; // only used for failure output message
    
    switch (derived) {
        case geometry_type::point:
            return detail::point_tests(geom);
        case geometry_type::linestring:
            return detail::linestring_tests(geom);
        case geometry_type::polygon:
            return detail::polygon_tests(geom);

        case geometry_type::geometry_collection:
            typestr = "geometry_collection";
            break;

        case geometry_type::multipoint:
            typestr = "multipoint";
            break;

        case geometry_type::multilinestring:
            typestr = "multilinestring";
            break;

        case geometry_type::multipolygon:
            typestr = "multipolygon";
            break;

        case geometry_type::geometry:
        default:
            typestr = "geometry";
            break;
    }

    FAIL() << "Tests for geometry type " + typestr + " are not implemented.";
}

} // namespace tests
} // namespace spatial
} // namespace ngen

#endif // NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP
