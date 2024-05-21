#pragma once

#include "DataProviderSelectors.hpp"
#include "ForcingsEngineDataProvider.hpp"

#include <utilities/mdarray.hpp>

namespace data_access {

struct ForcingsEngineLumpedDataProvider
  : public ForcingsEngineDataProvider<double, CatchmentAggrDataSelector>
{
    static constexpr auto bad_index = static_cast<std::size_t>(-1);

    ~ForcingsEngineLumpedDataProvider() override = default;

    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

    std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

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

    static ForcingsEngineDataProvider* lumped_instance(
        const std::string& init,
        const std::string& time_start,
        const std::string& time_end,
        const std::string& time_fmt = default_time_format
    );

  private:
    ForcingsEngineLumpedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds
    );

    /**
     * @brief Update to next timestep.
     * 
     * @return true
     * @return false 
     */
    bool next();

    /**
     * @brief Get a forcing value from the instance
     * 
     * @param divide_id Divide ID to index at
     * @param variable Forcings variable to get
     * @return double 
     */
    double at(
        const std::string& divide_id,
        const std::string& variable
    );

    /**
     * @brief Get a forcing value from the instance
     * 
     * @param divide_index Divide index
     * @param variable_index Variable index
     * @return double 
     */
    double at(
        std::size_t divide_idx,
        std::size_t variable_idx
    );

    /**
     * @brief Update the value storage.
     */
    void update_value_storage_();

    //! Divide index map
    //!     (Divide ID) -> (Divide ID Array Index)
    std::unordered_map<int, int> var_divides_{};

    /**
     * Values are stored indexed on (divide_id, variable),
     * such that the structure can be visualized as:
     *
     *   Divide ID : || D0       | D1       || D0  ...
     *   Variable  : || V1 V2 V3 | V1 V2 V3 || V1  ...
     *   Value     : ||  9 11 31 |  3  4  5 || 10  ...
     *
     * Some notes for future reference:
     * - Time complexity to update is approximately O(V*D),
     *   where V is the number of variables and D is the number of divides.
     *   In general, D will dominate and, V will be some small constant amount.
     */
    ngen::mdarray<double> var_cache_{};
};

} // namespace data_access
