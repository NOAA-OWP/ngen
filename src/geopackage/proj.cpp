#include "proj.hpp"
#include <boost/geometry/srs/projections/epsg.hpp>

namespace ngen {
namespace srs {

const epsg::srs_type epsg::epsg5070_ =
    epsg::srs_type(bg::srs::dpar::proj_aea)
                  (bg::srs::dpar::ellps_grs80)
                  (bg::srs::dpar::towgs84, {0,0,0,0,0,0,0})
                  (bg::srs::dpar::lat_0, 23)
                  (bg::srs::dpar::lon_0, -96)
                  (bg::srs::dpar::lat_1, 29.5)
                  (bg::srs::dpar::lat_2, 45.5)
                  (bg::srs::dpar::x_0, 0)
                  (bg::srs::dpar::y_0, 0);

const epsg::srs_type epsg::epsg3857_ =
    epsg::srs_type(bg::srs::dpar::proj_merc)
                  (bg::srs::dpar::units_m)
                  (bg::srs::dpar::no_defs)
                  (bg::srs::dpar::a, 6378137)
                  (bg::srs::dpar::b, 6378137)
                  (bg::srs::dpar::lat_ts, 0)
                  (bg::srs::dpar::lon_0, 0)
                  (bg::srs::dpar::x_0, 0)
                  (bg::srs::dpar::y_0, 0)
                  (bg::srs::dpar::k, 1);

auto epsg::get(uint32_t srid) -> srs_type
{
    switch (srid) {
        case 5070:
            return epsg5070_;
        case 3857:
            return epsg3857_;
        default:
            return bg::projections::detail::epsg_to_parameters(srid);
    }
}

} // namespace srs
} // namespace ngen
