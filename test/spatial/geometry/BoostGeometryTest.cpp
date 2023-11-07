#include "geometry_test_suite.hpp"
#include "geometry/backends/boost.hpp"

NGEN_GEOMETRY_TEST(BoostPointTest, ngen::spatial::backend::boost_point)
NGEN_GEOMETRY_TEST(BoostLineStringTest, ngen::spatial::backend::boost_linestring, {});
NGEN_GEOMETRY_TEST(BoostLinearRingTest, ngen::spatial::backend::boost_linearring, {});
// NGEN_GEOMETRY_TEST(BoostPolygonTest,    ngen::spatial::backend::boost_polygon);
