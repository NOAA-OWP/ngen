#pragma once

#include "Driver.hpp"
#include <boost/core/span.hpp>
#include <boost/geometry/util/type_traits.hpp>
#include <utilities/Feature.hpp>
#include <utilities/FeatureCollection.hpp>
#include <utilities/ngen_sqlite.hpp>
#include <utilities/EndianCopy.hpp>

namespace ngen {

struct GeoPackageDriver : public Driver<FeatureCollection> {
    FeatureCollection read(const std::string& input) override;

  private:

    enum wkb_t : uint32_t {
      geometry            = 0,
      point               = 1,
      linestring          = 2,
      polygon             = 3,
      multipoint          = 4,
      multilinestring     = 5,
      multipolygon        = 6,
      geometry_collection = 7
    };
    
    template<
        typename Geometry,
        std::enable_if_t<
            !boost::geometry::util::is_multi<Geometry>::value,
            bool
        > = true
    >
    static Geometry parse_geometry(boost::span<const uint8_t> buffer, int& index, uint8_t order);

    template<>
    auto parse_geometry(boost::span<const uint8_t> buffer, int& /* unused*/, uint8_t /* unused */)
      -> ngen::geometry
    {
        if (buffer.size() < 5) {
            throw std::runtime_error{"buffer reached end before encountering WKB"};
        }

        int index = 0;
        const uint8_t order = buffer[index];
        index++;

        uint32_t type = 0;
        utils::copy_from(buffer, index, type, order);

        switch(static_cast<wkb_t>(type)) {
          case wkb_t::point:
              return parse_geometry<ngen::point>(buffer, index, order);
          case wkb_t::linestring:
              return parse_geometry<ngen::linestring>(buffer, index, order);
          case wkb_t::polygon:
              return parse_geometry<ngen::polygon>(buffer, index, order);
          case wkb_t::multipoint:
              return parse_geometry<ngen::multipoint>(buffer, index, order);
          case wkb_t::multilinestring:
              return parse_geometry<ngen::multilinestring>(buffer, index, order);
          case wkb_t::multipolygon:
              return parse_geometry<ngen::multipolygon>(buffer, index, order);
          default:
              throw std::runtime_error(
                  "this reader only implements OGC geometry types 1-6, "
                  "but received type " + std::to_string(type)
              );
        }
    }

    template<>
    auto parse_geometry(boost::span<const uint8_t> buffer, int& index, uint8_t order)
      -> ngen::point
    {
        double x = 0.0;
        double y = 0.0;
        utils::copy_from(buffer, index, x, order);
        utils::copy_from(buffer, index, y, order);
        return ngen::point{x, y};
    }

    template<>
    auto parse_geometry(boost::span<const uint8_t> buffer, int& index, uint8_t order)
      -> ngen::linestring
    {
        uint32_t count = 0;
        utils::copy_from(buffer, index, count, order);

        ngen::linestring linestring;
        linestring.resize(count);
        for (auto& child : linestring) {
            child = parse_geometry<ngen::point>(buffer, index, order);
        }

        return linestring;
    }

    template<>
    auto parse_geometry(boost::span<const uint8_t> buffer, int& index, uint8_t order)
      -> ngen::polygon
    {
        uint32_t count = 0;
        utils::copy_from(buffer, index, count, order);

        ngen::polygon polygon;
        if (count > 1) {
            // polygons only have 1 outer ring,
            // so any extra vectors are considered to be
            // inner rings.
            polygon.inners().resize(count - 1);
        }

        const auto outer = parse_geometry<ngen::linestring>(buffer, index, order);
        polygon.outer().reserve(outer.size());
        for (const auto& p : outer) {
            polygon.outer().push_back(p);
        }

        for (uint32_t i = 0; i < count - 1; i++) {
            const auto inner = parse_geometry<ngen::linestring>(buffer, index, order);
            auto& polygon_inner = polygon.inners()[i];
            polygon_inner.reserve(inner.size());
            for (const auto& p : inner) {
                polygon_inner.emplace_back(p);
            }
        }

        return polygon;
    }

    template<
        typename MultiGeometry,
        std::enable_if_t<
            boost::geometry::util::is_multi<MultiGeometry>::value,
            bool
        > = true
    >
    static auto parse_geometry(boost::span<const uint8_t> buffer, int& index, uint8_t order)
      -> MultiGeometry
    {
        uint32_t count = 0;
        utils::copy_from(buffer, index, count, order);

        MultiGeometry multi;
        multi.resize(count);

        for (auto& child : multi) {
            const uint8_t new_order = buffer[index];
            index++;

            uint32_t type = 0;
            utils::copy_from(buffer, index, type, new_order);
            // throw_if_not_type(type, wkb_geom_t::...)

            child = parse_geometry<typename MultiGeometry::value_type>(buffer, index, new_order);
        }

        return multi;
    }
};

} // namespace ngen
