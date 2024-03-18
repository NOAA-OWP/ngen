#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <boost/core/span.hpp>

#include "Bmi_Py_Adapter.hpp"
#include <utilities/mdarray.hpp>

namespace data_access {

struct ForcingsEngine
{
    using size_type                 = std::size_t;
    using clock_type                = std::chrono::system_clock;
    static constexpr auto bad_index = static_cast<size_type>(-1);

    void set_communicator(int handle);

    auto outputs() const noexcept
      -> boost::span<const std::string>;

    auto time_begin() const noexcept
      -> const clock_type::time_point&;

    auto time_end() const noexcept
      -> const clock_type::time_point&;

    auto time_step() const noexcept
      -> const clock_type::duration&;

    auto time_index(time_t ctime) const noexcept
      -> size_type;

    auto divide_index(const std::string& divide_id) const noexcept
      -> size_type;

    auto variable_index(const std::string& variable) const noexcept
      -> size_type;

    auto at(
        const time_t& raw_time,
        const std::string& divide_id,
        const std::string& variable
    ) -> double;

    /**
     * Get an instance of the Forcings Engine
     * 
     * @param init Path to initialization file
     * @param time_start Starting time
     * @param time_end Ending time
     * @return ForcingsEngine& Instance of the Forcings Engine
     *                         that will destruct at program
     *                         termination.
     */
    static ForcingsEngine& instance(
        const std::string& init,
        std::size_t time_start,
        std::size_t time_end
    );

    ForcingsEngine(const std::string& init, size_type time_start, size_type time_end);

    static std::unique_ptr<ForcingsEngine> inst_;

    ~ForcingsEngine() {
        bmi_->Finalize();
    }

  private:

    static void check_runtime_requirements();

    ForcingsEngine() = default;

    std::unique_ptr<models::bmi::Bmi_Py_Adapter> bmi_ = nullptr;
    clock_type::time_point                       time_start_{};
    clock_type::time_point                       time_end_{};
    clock_type::duration                         time_step_{};
    size_type                                    time_current_index_{};
    std::vector<std::string>                     var_outputs_{};
    std::vector<int>                             var_divides_{};

    /**
     * Flat value cache vector.
     * 
     * Values are stored indexed on (timestep, divide_id, variable),
     * such that the structure can be visualized as:
     *
     *   Timestep  : || T0       |          || T1  ...
     *   Divide ID : || D0       | D1       || D0  ...
     *   Variable  : || V1 V2 V3 | V1 V2 V3 || V1  ...
     *   Value     : ||  9 11 31 |  3  4  5 || 10  ...
     *
     * Some notes for future reference:
     * - Time complexity to update is approximately O(T*V*D),
     *   where T = T2 - T1 is the number of timesteps between
     *   two time points, V is the number of variables
     *   and D is the number of divides. In general, D will dominate;
     *   T will usually be 1, if updated at each time event; and, V will be
     *   some small constant amount. Meaning, we should have about O(V*D).
     */
    ngen::mdarray<double> var_cache_{};
};

} // namespace data_access
