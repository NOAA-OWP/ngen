#include <gtest/gtest.h>
#include <mmap_allocator.hpp>
#include <numeric>

TEST(mmap_allocator_Test, basicPool) {
    using alloc = ngen::mmap_allocator<double, ngen::basic_pool>;
    std::vector<double, alloc> mmap_vector(5);

    EXPECT_EQ(mmap_vector.capacity(), 5);

    std::iota(mmap_vector.begin(), mmap_vector.end(), 0);

    size_t i = 0;
    for (auto& v : mmap_vector) {
        EXPECT_NEAR(i++, v, 1e-6);
    }

    ASSERT_NO_THROW(mmap_vector.reserve(15));
    EXPECT_EQ(mmap_vector.capacity(), 15);
}
