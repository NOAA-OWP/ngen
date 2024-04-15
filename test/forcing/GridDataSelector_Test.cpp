#include <gtest/gtest.h>

#include <cmath>

#include <forcing/DataProvider.hpp>
#include <forcing/GridDataSelector.hpp>

// A fake grid data provider containing a 10x10 uniform grid
// with x/y indices in [0, 10). Temporal domain is [0, 1 hour).
struct TestGridDataProvider
  : public data_access::DataProvider<Cell, GridDataSelector>
{
    TestGridDataProvider()
    {
        values_.reserve(cols_ * rows_);
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                values_[j + (i * cols_)] = static_cast<double>(i + j);
            }
        }
    }

    boost::span<const std::string> get_available_variable_names() override
    { return { &variable_, 1 }; }

    long get_data_start_time() override
    { return 0; }

    long get_data_stop_time() override
    { return get_data_start_time() + record_duration(); }

    long record_duration() override
    { return 3600; }

    size_t get_ts_index_for_time(const time_t& epoch_time) override
    { return -1; }

    Cell get_value(const GridDataSelector& selector, data_access::ReSampleMethod method) override
    { return {}; };

    std::vector<Cell> get_values(const GridDataSelector& selector, data_access::ReSampleMethod method) override
    {
        const auto start = selector.initial_time();
        const auto end   = selector.initial_time() + selector.duration();

        if (start < get_data_start_time() || start >= get_data_stop_time()) {
            throw std::out_of_range{"Starting time out of range"};
        }

        if (end < get_data_start_time() || end >= get_data_stop_time()) {
            throw std::out_of_range{"Ending time out of range"};
        }

        const auto scells = selector.cells();
        std::vector<Cell> result{scells.begin(), scells.end()};
        
        for (auto& cell : result) {
            if (cell.x < 0 || cell.x >= cols_) {
                throw std::out_of_range{"Column " + std::to_string(cell.x) + " out of range of " + std::to_string(cols_)};
            }

            if (cell.y < 0 || cell.y >= rows_) {
                throw std::out_of_range{"Row " + std::to_string(cell.y) + " out of range of " + std::to_string(rows_)};
            }

            cell.value = values_[cell.x + (cell.y * cols_)];
        }

        return result;
    }

  private:
    static const std::string variable_;
    size_t cols_ = 10;
    size_t rows_ = 10;
    std::vector<double> values_;
};

const std::string TestGridDataProvider::variable_ = "variable";

// x is column
// y is row
inline Cell make_cell_xy(std::uint64_t x, std::uint64_t y)
{ return { x, y, static_cast<uint64_t>(-1), NAN}; }

TEST(GridDataSelectorTest, Example) {
    TestGridDataProvider provider{};
    GridDataSelector selector{
        "variable",             // variable
        0,                      // init_time
        3599,                   // duration
        "m",                    // units
        {{                      // cells
            make_cell_xy(0, 0),
            make_cell_xy(5, 2),
            make_cell_xy(9, 9)
        }}
    };
    
    const auto cells = provider.get_values(selector, data_access::ReSampleMethod::SUM);

    const auto expect_cell = [&cells](std::size_t index, std::uint64_t x, std::uint64_t y)
    {
        const auto& cell = cells[index];
        EXPECT_EQ(cell.x, x);
        EXPECT_EQ(cell.y, y);
        EXPECT_EQ(cell.z, static_cast<uint64_t>(-1));
        EXPECT_EQ(cell.value, static_cast<double>(x + y));
    };
    
    ASSERT_EQ(cells.size(), 3);
    expect_cell(/*index=*/ 0, /*x=*/ 0, /*y=*/ 0);
    expect_cell(/*index=*/ 1, /*x=*/ 5, /*y=*/ 2);
    expect_cell(/*index=*/ 2, /*x=*/ 9, /*y=*/ 9);
}
