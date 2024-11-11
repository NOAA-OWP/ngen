#include "proj.hpp"
#include "Logger.hpp"

namespace ngen {
namespace srs {

const epsg::def_type epsg::defs_ = {
    {4326, epsg::srs_type(bg::srs::dpar::proj_longlat)(bg::srs::dpar::ellps_wgs84)(bg::srs::dpar::datum_wgs84)(bg::srs::dpar::no_defs)},
    {5070, epsg::srs_type(bg::srs::dpar::proj_aea)(bg::srs::dpar::ellps_grs80)(bg::srs::dpar::towgs84, {0,0,0,0,0,0,0})(bg::srs::dpar::lat_0, 23)(bg::srs::dpar::lon_0, -96)(bg::srs::dpar::lat_1, 29.5)(bg::srs::dpar::lat_2, 45.5)(bg::srs::dpar::x_0, 0)(bg::srs::dpar::y_0, 0)},
    {3857, epsg::srs_type(bg::srs::dpar::proj_merc)(bg::srs::dpar::units_m)(bg::srs::dpar::no_defs)(bg::srs::dpar::a, 6378137)(bg::srs::dpar::b, 6378137)(bg::srs::dpar::lat_ts, 0)(bg::srs::dpar::lon_0, 0)(bg::srs::dpar::x_0, 0)(bg::srs::dpar::y_0, 0)(bg::srs::dpar::k, 1)}
};

auto epsg::get(uint32_t srid) -> srs_type
{

    if (defs_.count(srid) == 0) {
        Logger::logMsgAndThrowError("SRID " + std::to_string(srid) + " is not supported. Project the input data to EPSG:5070 or EPSG:4326.");
    }

    return defs_.at(srid);
}

} // namespace srs
} // namespace ngen
