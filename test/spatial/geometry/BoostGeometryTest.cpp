#include "geometry_test_suite.hpp"
#include "geometry/backends/boost.hpp"

namespace backend = ngen::spatial::backend;

#define NGEN_BOOST_GEOMETRY_TEST(TestName, TypeName, ...) \
    NGEN_GEOMETRY_TEST(                                   \
        TestName,                                         \
        TypeName,                                         \
        backend::boost_point,                             \
        backend::boost_linearring,                        \
        ##__VA_ARGS__                                     \
    )

NGEN_BOOST_GEOMETRY_TEST(BoostPointTest, backend::boost_point)
NGEN_BOOST_GEOMETRY_TEST(BoostLineStringTest, backend::boost_linestring, {});
NGEN_BOOST_GEOMETRY_TEST(BoostLinearRingTest, backend::boost_linearring, {});
NGEN_BOOST_GEOMETRY_TEST(BoostPolygonTest,    backend::boost_polygon);
