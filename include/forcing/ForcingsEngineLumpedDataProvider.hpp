#pragma once

#include "DataProviderSelectors.hpp"
#include "ForcingsEngineDataProvider.hpp"

#include <utilities/mdarray.hpp>

namespace data_access {

struct ForcingsEngineLumpedDataProvider
  : public ForcingsEngineDataProvider<double, CatchmentAggrDataSelector>
{
    static constexpr auto bad_index = static_cast<std::size_t>(-1);

    ForcingsEngineLumpedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds
    );

    ForcingsEngineLumpedDataProvider(
        const std::string& init,
        const std::string& time_start,
        const std::string& time_end,
        const std::string& time_fmt = "%Y-%m-%d %H:%M:%S"
    );

    ~ForcingsEngineLumpedDataProvider() override = default;

    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

    std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

  private:

    /**
     * @brief Update to next timestep.
     * 
     * @return true
     * @return false 
     */
    bool next();

    /**
     * @brief Get the index in `CAT-ID` for a given divide in the instance cache.
     * @note The `CAT-ID` output variable uses integer values instead of strings.
     * 
     * @param divide_id A hydrofabric divide ID, i.e. "cat-*"
     * @return size_type 
     */
    std::size_t divide_index(const std::string& divide_id) noexcept;

    /**
     * @brief Get the index of a variable in the instance cache.
     * 
     * @param variable 
     * @return size_type 
     */
    std::size_t variable_index(const std::string& variable) noexcept;

    /**
     * @brief Get a forcing value from the instance
     * 
     * @param divide_id Divide ID to index at
     * @param variable Forcings variable to get
     * @param previous If true, return the previous timestep values.
     * @return double 
     */
    double at(
        const std::string& divide_id,
        const std::string& variable,
        bool previous = false
    );

    /**
     * @brief Get a forcing value from the instance
     * 
     * @param divide_index Divide index
     * @param variable_index Variable index
     * @param previous If true, return the previous timestep values.
     * @return double 
     */
    double at(
        std::size_t divide_idx,
        std::size_t variable_idx,
        bool previous = false
    );

    /**
     * @brief Update the value storage.
     */
    void update_value_storage_();


    //! Available divide IDs
    std::vector<int> var_divides_{};

    /**
     * Values are stored indexed on (2, divide_id, variable),
     * such that the structure can be visualized as:
     *
     *   Divide ID : || D0       | D1       || D0  ...
     *   Variable  : || V1 V2 V3 | V1 V2 V3 || V1  ...
     *   Value     : ||  9 11 31 |  3  4  5 || 10  ...
     *
     * Some notes for future reference:
     * - Time complexity to update is approximately O(2*V*D) = O(V*D),
     *   where V is the number of variables and D is the number of divides.
     *   In general, D will dominate and, V will be some small constant amount.
     */
    ngen::mdarray<double> var_cache_{};
};

} // namespace data_access
