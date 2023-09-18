#include <gtest/gtest.h>
#include <mmap_allocator.hpp>
#include <numeric>

TEST(mmap_allocator_Test, allocation) {
    using alloc = ngen::mmap_allocator<double, ngen::basic_pool>;

    alloc mmapper{"/home/jsinghm/Documents/ngen/"};
    std::vector<double, alloc> mmap_vector(5, mmapper);

    EXPECT_EQ(mmap_vector.capacity(), 5);

    std::iota(mmap_vector.begin(), mmap_vector.end(), 0);

    for (size_t i = 0; i < mmap_vector.size(); i++) {
        EXPECT_EQ(i, mmap_vector[i]);
    }

    // ASSERT_NO_THROW(mmap_vector.reserve(10));
    // EXPECT_EQ(mmap_vector.capacity(), 10);
}
