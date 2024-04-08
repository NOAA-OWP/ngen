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

    ForcingsEngine(const std::string& init, size_type time_start, size_type time_end);

    ~ForcingsEngine();

    /**
     * @brief Get the output variables associated with this instance of the
     *        Forcings Engine
     * 
     * @return boost::span<const std::string> 
     */
    boost::span<const std::string> outputs() const noexcept;

    /**
     * @brief Get the beginning time point for this instance of the
     *        Forcings Engine
     * 
     * @return const clock_type::time_point& 
     */
    const clock_type::time_point& time_begin() const noexcept;

    /**
     * @brief Get the ending time point for this instance of the
     *        Forcings Engine
     * 
     * @return const clock_type::time_point& 
     */
    const clock_type::time_point& time_end() const noexcept;

    /**
     * @brief Get the stride/time step this instance of the
     *        Forcings Engine follows.
     * 
     * @return const clock_type::duration& 
     */
    const clock_type::duration& time_step() const noexcept;

    /**
     * @brief Get the index for a C-style epoch within this instance of the
     *        Forcings Engine's temporal domain.
     * 
     * @param ctime C-style epoch
     * @return size_type, where the index represents a point between time_begin() and time_end() by time_step().
     */
    size_type time_index(time_t ctime) const noexcept;

    /**
     * @brief Get the index for a std::chrono::time_point within this instance of the
     *        Forcings Engine's temporal domain.
     * 
     * @param epoch std::chrono::time_point
     * @return size_type, where the index represents a point between time_begin() and time_end() by time_step().
     */
    size_type time_index(clock_type::time_point epoch) const noexcept;

    /**
     * @brief Get the index in `CAT-ID` for a given divide in the instance cache.
     * @note The `CAT-ID` output variable uses integer values instead of strings.
     * 
     * @param divide_id A hydrofabric divide ID, i.e. "cat-*"
     * @return size_type 
     */
    size_type divide_index(const std::string& divide_id) const noexcept;

    /**
     * @brief Get the index of a variable in the instance cache.
     * 
     * @param variable 
     * @return size_type 
     */
    size_type variable_index(const std::string& variable) const noexcept;

    bool next();

    /**
     * @brief Get a forcing value from the instance
     * 
     * @param raw_time Epoch for the time to get
     * @param divide_id Divide ID to index at
     * @param variable Forcings variable to get
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
     * @param time_index Time index
     * @param divide_index Divide index
     * @param variable_index Variable index
     * @return double 
     */
    double at(
        size_type divide_idx,
        size_type variable_idx,
        bool previous = false
    );

    /**
     * @brief Finalize a given Forcings Engine instance.
     */
    void finalize();

    /**
     * @brief Finalize all Forcings Engine instances.
     */
    static void finalize_all();

    /**
     * @brief Set the MPI communicator that the Forcings Engine should use.
     *
     * @param handle int representing a communicator handle.
     */
    // void set_communicator(int handle);

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

    /**
     * @brief Checks that the $WGRIB2 environment variable is set and
     *        that the NWM Forcings Engine python module is loadable.
     *
     * @note By loading the python module, this also checks for an
     *       installation of ESMF (since it's imported at load).
     */
    static void check_runtime_requirements();

  private:

    /**
     * @brief Update the value storage.
     */
    void update_value_storage_();

    ForcingsEngine() = default;

    // Available instances of Forcings Engines. This will vary
    // if multiple instance types are used, i.e. hydrofabric, unstructured, or gridded.
    static std::unordered_map<
        std::string,
        std::unique_ptr<ForcingsEngine>
    > instances_;

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
