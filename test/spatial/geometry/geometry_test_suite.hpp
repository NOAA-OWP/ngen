#ifndef NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP

#include <gtest/gtest.h>

#include <geometry/geometry.hpp>
#include <geometry/point.hpp>
#include <geometry/linestring.hpp>
#include <geometry/polygon.hpp>

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

NGEN_FORCE_INLINE void linestring_tests() {};
NGEN_FORCE_INLINE void polygon_tests() {};

} // namespace detail

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
            return detail::linestring_tests();
        case geometry_type::polygon:
            return detail::polygon_tests();

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
