#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/core/span.hpp>

struct Cell {
    std::uint64_t x;
    std::uint64_t y;
    std::uint64_t z;
    double value;
};

struct GridDataSelector {

    GridDataSelector(
        std::string variable,
        time_t init_time,
        long duration,
        std::string units,
        boost::span<const Cell> cells
    )
      : init_(init_time)
      , duration_seconds_(duration)
      , variable_(std::move(variable))
      , output_units_(std::move(units))
      , cells_(cells.begin(), cells.end())
    {}

    GridDataSelector() noexcept = default;
    virtual ~GridDataSelector() = default;

    time_t& initial_time() noexcept { return init_; }
    time_t initial_time() const noexcept { return init_; }
    long& duration() noexcept { return duration_seconds_; }
    long duration() const noexcept { return duration_seconds_; }
    std::string& variable() noexcept { return variable_; }
    const std::string& variable() const noexcept { return variable_; }
    std::string& units() noexcept { return output_units_; }
    const std::string& units() const noexcept { return output_units_; }
    boost::span<Cell> cells() noexcept { return cells_; }
    boost::span<const Cell> cells() const noexcept { return cells_; }

  private:
    //! Initial time for query.
    //! @todo Refactor to use std::chrono
    time_t init_;

    //! Duration for query, in seconds.
    //! @todo Refactor to use std::chrono
    long duration_seconds_;

    //! Variable to return from query.
    std::string variable_;

    //! Units for output variable.
    std::string output_units_;

    //! Cells to gather.
    std::vector<Cell> cells_;
};
