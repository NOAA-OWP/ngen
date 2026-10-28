#include <memory>
#include <stdexcept>
#include <string>

#include "gtest/gtest.h"

#include <FeatureBuilder.hpp>
#include <FileChecker.h>
#include <HY_Features_Attributes.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>
#include <features/Features.hpp>

/**
 * Reading the hydrofabric attributes a catchment is realized from.
 *
 * GeoPackage is the current input format and GeoJSON remains supported. Both are read into the
 * same geojson::Feature, in WGS84 either way, so these cases stand for both; what differs between
 * them is how an attribute is spelled, which is the subject of the first cases here.
 */
class HY_Features_Attributes_Test : public ::testing::Test {
  protected:
    /** A square degree of WGS84, as a ring boost::geometry will accept. */
    static geojson::polygon_t unitSquare() {
        geojson::polygon_t polygon;
        boost::geometry::append(polygon.outer(), geojson::coordinate_t(0.0, 0.0));
        boost::geometry::append(polygon.outer(), geojson::coordinate_t(0.0, 1.0));
        boost::geometry::append(polygon.outer(), geojson::coordinate_t(1.0, 1.0));
        boost::geometry::append(polygon.outer(), geojson::coordinate_t(1.0, 0.0));
        boost::geometry::append(polygon.outer(), geojson::coordinate_t(0.0, 0.0));
        return polygon;
    }

    /** A catchment feature carrying the given properties and a single-polygon geometry. */
    static geojson::Feature polygonFeature(geojson::PropertyMap properties) {
        return std::make_shared<geojson::PolygonFeature>(
                geojson::PolygonFeature(unitSquare(), "cat-27", std::move(properties)));
    }

    /** A catchment feature with the given properties and a two-ring multipolygon face. */
    static geojson::Feature multiPolygonFeature(geojson::PropertyMap properties) {
        geojson::multipolygon_t two{unitSquare(), unitSquare()};
        return std::make_shared<geojson::MultiPolygonFeature>(
                geojson::MultiPolygonFeature(two, "cat-27", std::move(properties)));
    }

    /** A catchment feature with the given properties and no face to measure. */
    static geojson::Feature facelessFeature(geojson::PropertyMap properties) {
        return std::make_shared<geojson::PointFeature>(
                geojson::PointFeature(geojson::coordinate_t(1.0, 2.0), "cat-27", std::move(properties)));
    }

    static geojson::PropertyMap withArea(const std::string& key, double value) {
        geojson::PropertyMap properties;
        properties.emplace(key, geojson::JSONProperty(key, value));
        return properties;
    }
};

/** The hydrofabric v1 spelling of the area attribute is read. */
TEST_F(HY_Features_Attributes_Test, AreaIsReadUnderTheV1Spelling)
{
    EXPECT_EQ(hy_features::attributes::area_from_attribute(polygonFeature(withArea("areasqkm", 15.625))),
              15.625);
}

/** The hydrofabric v2 spelling of the area attribute is read, so either vintage resolves. */
TEST_F(HY_Features_Attributes_Test, AreaIsReadUnderTheV2Spelling)
{
    EXPECT_EQ(hy_features::attributes::area_from_attribute(polygonFeature(withArea("area_sqkm", 15.625))),
              15.625);
}

/**
 * A catchment carrying both spellings is read as the v2 one.
 *
 * A hydrofabric that has been through the v1 to v2 rename may carry each, and v2 is the one a
 * current fabric means.
 */
TEST_F(HY_Features_Attributes_Test, TheV2SpellingWinsWhenBothArePresent)
{
    geojson::PropertyMap both;
    both.emplace("areasqkm", geojson::JSONProperty("areasqkm", 1.0));
    both.emplace("area_sqkm", geojson::JSONProperty("area_sqkm", 15.625));

    EXPECT_EQ(hy_features::attributes::area_from_attribute(polygonFeature(both)), 15.625);
}

/** The attribute is what is used when a catchment carries both an attribute and a divide. */
TEST_F(HY_Features_Attributes_Test, AttributeIsPreferredByDefault)
{
    EXPECT_EQ(hy_features::attributes::area_sqkm(polygonFeature(withArea("areasqkm", 15.625))), 15.625);
}

/** Asking for the divide instead measures it, rather than taking the attribute beside it. */
TEST_F(HY_Features_Attributes_Test, GeometryIsUsedWhenItIsTheOneAskedFor)
{
    geojson::Feature feature = polygonFeature(withArea("areasqkm", 15.625));
    double measured = hy_features::attributes::area_sqkm(
            feature, hy_features::attributes::AreaSource::geometry);

    EXPECT_NE(measured, 15.625);
    EXPECT_EQ(measured, *hy_features::attributes::area_from_geometry(*feature->geometry()));
}

/** A catchment with a divide but no area attribute is measured rather than refused. */
TEST_F(HY_Features_Attributes_Test, DivideAnswersWhenTheAttributeIsAbsent)
{
    EXPECT_GT(hy_features::attributes::area_sqkm(polygonFeature({})), 0.0);
}

/** A catchment with an attribute but no divide is read rather than refused. */
TEST_F(HY_Features_Attributes_Test, AttributeAnswersWhenTheDivideIsAbsent)
{
    EXPECT_EQ(hy_features::attributes::area_sqkm(facelessFeature(withArea("areasqkm", 15.625)),
                                                 hy_features::attributes::AreaSource::geometry),
              15.625);
}

/** Only a catchment with neither source stops the run, and it is named in the message. */
TEST_F(HY_Features_Attributes_Test, NeitherSourceIsRefused)
{
    try {
        hy_features::attributes::area_sqkm(facelessFeature({}));
        FAIL() << "expected a catchment with no area attribute and no divide to be refused";
    }
    catch (const std::invalid_argument& e) {
        EXPECT_NE(std::string(e.what()).find("cat-27"), std::string::npos) << e.what();
    }
}

/** Both shapes a face may be described as are measured, so neither has to be converted first. */
TEST_F(HY_Features_Attributes_Test, EitherArealShapeIsMeasured)
{
    const double one = *hy_features::attributes::area_from_geometry(
            *polygonFeature({})->geometry());
    const double two = *hy_features::attributes::area_from_geometry(
            *multiPolygonFeature({})->geometry());

    EXPECT_GT(one, 0.0);
    EXPECT_DOUBLE_EQ(two, 2 * one);
}

/** A geometry that bounds nothing has no area, rather than a meaningless one. */
TEST_F(HY_Features_Attributes_Test, ANonArealGeometryIsNotAnArea)
{
    EXPECT_FALSE(hy_features::attributes::area_from_geometry(
            *facelessFeature({})->geometry()).has_value());
}

/** A face wound the wrong way, or with no extent, is no more an area than no face at all. */
TEST_F(HY_Features_Attributes_Test, ADegenerateFaceIsNotAnArea)
{
    EXPECT_FALSE(hy_features::attributes::area_from_geometry(
            geojson::geometry{geojson::polygon_t{}}).has_value());
}
