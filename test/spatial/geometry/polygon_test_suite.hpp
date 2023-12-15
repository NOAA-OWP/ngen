#ifndef NGEN_TEST_SPATIAL_GEOMETRY_POLYGON_TEST_SUITE_HPP
#define NGEN_TEST_SPATIAL_GEOMETRY_POLYGON_TEST_SUITE_HPP

#include <gtest/gtest.h>
#include <geometry/polygon.hpp>

namespace ngen {
namespace tests {

template<typename DerivedPointType, typename DerivedRingType>
void polygon_tests(ngen::spatial::geometry* geom)
{
    static testing::internal::Random rng{1234};
    static auto ptr = dynamic_cast<ngen::spatial::polygon*>(geom);

    ASSERT_EQ(ptr->type(), ngen::spatial::geometry_t::polygon);

    // Test copy semantics
    auto ring = DerivedRingType{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 0}};
    ASSERT_NO_THROW(ptr->outer().clone(ring));
    ASSERT_EQ(ptr->outer().size(), 5);
    const auto& pt = ptr->outer().front();
    EXPECT_NEAR(pt.x(), ring.get(0).x(), 1e-6);
    EXPECT_NEAR(pt.y(), ring.get(0).y(), 1e-6);

    // Test move semantics
    ring = DerivedRingType{};
    ASSERT_NO_THROW(ptr->outer().swap(ring));
    ASSERT_EQ(ptr->outer().size(), 0);
    ASSERT_EQ(ring.size(), 5);
};

} // namespace tests
} // namespace ngen

#endif // NGEN_TEST_SPATIAL_GEOMETRY_POLYGON_TEST_SUITE_HPP
