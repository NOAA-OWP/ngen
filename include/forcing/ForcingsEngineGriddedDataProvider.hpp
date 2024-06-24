#pragma once

#include "GridDataSelector.hpp"
#include "ForcingsEngineDataProvider.hpp"

namespace data_access {

struct ForcingsEngineGriddedDataProvider
  : public ForcingsEngineDataProvider<double, GridDataSelector>
{
    using data_type = data_type;
    using selection_type = selection_type;
    using base_type = ForcingsEngineDataProvider<data_type, selection_type>;

    /**
     * Construct a domain-wide Gridded Forcings Engine data provider
     * @param init Path to instance initialization file
     * @param time_begin_seconds Time in seconds for begin time. Typically 0.
     * @param time_end_seconds Time in seconds for end time. Typically the lifetime of the simulation.
     */
    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds
    );

    /**
     * Construct a masked Gridded Forcings Engine data provider
     * @param mask Bounding box used to mask data provider, results will be returned
     *             within this region.
     */
    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds,
        const BoundingBox& mask
    );

    /**
     * Construct a polygon masked Gridded Forcings Engine data provider
     * @param boundary Polygon used to mask data provider, results will be returned
     *                 within the **bounding box** of this region.
     */
    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds,
        const geojson::polygon_t& boundary
    );

    ~ForcingsEngineGriddedDataProvider() override = default;

    data_type get_value(
      const selection_type& selector,
      data_access::ReSampleMethod m
    ) override;

    /**
     * Get the values of a gridded variable in time.
     */
    std::vector<data_type> get_values(
      const selection_type& selector,
      data_access::ReSampleMethod m
    ) override;

  private:
    /**
     * Get the underlying grid specification of this provider's instance.
     */
    const GridSpecification& grid() const noexcept;

    /**
     * Get the mask of this provider. If the provider is domain-wide, the
     * returned bounding box will equivalent to the grid's bounding box.
     */
    const BoundingBox& mask() const noexcept;


    //! Grid ID for underlying forcings engine instance
    int var_grid_id_ = -1;

    //! Total grid specification for forcings engine instance
    GridSpecification var_grid_{};

    //! Provider Grid Mask, the AOI this provider operates under
    BoundingBox var_grid_mask_{};
};

} // namespace data_access
