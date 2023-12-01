#ifndef NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP

#include <gtest/gtest.h>

#include "point_test_suite.hpp"
#include "linestring_test_suite.hpp"
#include "polygon_test_suite.hpp"

#define NGEN_GEOMETRY_TEST(TestName, TypeName, DerivedPoint, DerivedRing, ...) \
    TEST(SpatialGeometry, TestName) {                                    \
        TypeName x{__VA_ARGS__};                                         \
        ngen::tests::perform_geometry_tests<DerivedPoint, DerivedRing>(std::addressof(x)); \
    }

namespace ngen {
namespace tests {

//! Apply test suites to an abstract geometry dependent on its derived type.
template<typename DerivedPointType, typename DerivedRingType>
void perform_geometry_tests(ngen::spatial::geometry* geom)
{
    if (geom == nullptr) {
        FAIL() << "No geometry not assigned to this instance of GeometryTest";
    }

    using geometry_type = ngen::spatial::geometry_t;

    geometry_type derived = geom->type();

    std::string typestr; // only used for failure output message
    
    switch (derived) {
        case geometry_type::point:
            return point_tests(geom);
        case geometry_type::linestring:
            return linestring_tests<DerivedPointType>(geom);
        case geometry_type::polygon:
            return polygon_tests<DerivedPointType, DerivedRingType>(geom);
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
} // namespace ngen

#endif // NGEN_TEST_SPATIAL_GEOMETRY_GEOMETRYTEST_HPP
