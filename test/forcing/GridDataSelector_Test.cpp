#include <gtest/gtest.h>

#include <cmath>

#include <forcing/DataProvider.hpp>
#include <forcing/GridDataSelector.hpp>

// A fake grid data provider containing a 10x10 uniform grid
// with x/y indices in [0, 10). Temporal domain is [0, 1 hour).
struct TestGridDataProvider
  : public data_access::DataProvider<Cell, GridDataSelector>
{
    explicit TestGridDataProvider(GridSpecification spec)
      : spec_(spec)
    {
        initialize_();
    }

    TestGridDataProvider()
      : TestGridDataProvider(GridSpecification{10, 10, {0, 10, 0, 10}})
    {}

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
            if (cell.x < 0 || cell.x >= spec_.columns) {
                throw std::out_of_range{"Column " + std::to_string(cell.x) + " out of range of " + std::to_string(spec_.columns)};
            }

            if (cell.y < 0 || cell.y >= spec_.rows) {
                throw std::out_of_range{"Row " + std::to_string(cell.y) + " out of range of " + std::to_string(spec_.rows)};
            }

            cell.value = values_[cell.x + (cell.y * spec_.columns)];
        }

        return result;
    }

    static const SelectorConfig default_selector;

  private:

    void initialize_() noexcept {
        values_.reserve(spec_.columns * spec_.rows);
        for (size_t i = 0; i < spec_.rows; ++i) {
            for (size_t j = 0; j < spec_.columns; ++j) {
                values_[j + (i * spec_.columns)] = static_cast<double>(i + j);
            }
        }
    }

    static const std::string variable_;
    GridSpecification spec_;
    std::vector<double> values_;
};

const std::string TestGridDataProvider::variable_ = "variable";
const SelectorConfig TestGridDataProvider::default_selector = {
    0, // init_time
    3599, // duration
    "variable", // variable
    "m", // units
};

// x is column
// y is row
inline Cell make_cell_xy(std::uint64_t x, std::uint64_t y)
{ return { x, y, static_cast<uint64_t>(-1), NAN}; }

TEST(GridDataSelectorTest, CellSelection) {
    TestGridDataProvider provider{};
    GridDataSelector selector{
        TestGridDataProvider::default_selector,
        {{ // cells
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

TEST(GridDataSelectorTest, ExtentSelection) {

    GridSpecification grid_spec {
        10, // rows 
        10, // cols
        {20, 30, 20, 30}
    };

    TestGridDataProvider provider{grid_spec};

    // Only take the upper-right 25 cells.
    GridDataSelector selector{
        TestGridDataProvider::default_selector,
        grid_spec,
        { 25, 30, 25, 30}
    };

    const auto cells = provider.get_values(selector, data_access::ReSampleMethod::SUM);
    ASSERT_GT(cells.size(), 0);
    EXPECT_EQ(cells.size(), 25);

    for (const auto& cell : cells) {
        EXPECT_LE(cell.x, 10);
        EXPECT_GE(cell.x, 0);
        EXPECT_LE(cell.y, 10);
        EXPECT_GE(cell.y, 0);
    }
}
