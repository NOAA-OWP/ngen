#include <gtest/gtest.h>
#include <wkb/reader.hpp>

using namespace geopackage;

class WKB_Test : public ::testing::Test
{
  protected:
    WKB_Test() {}

    ~WKB_Test() override {}

    void SetUp() override {
        this->wkb = {
            0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x05, 0x00, 0x00, 0x00, 0x54, 0xE3, 0xA5,
            0x9B, 0xC4, 0x60, 0x25, 0x40, 0x64, 0x3B, 0xDF,
            0x4F, 0x8D, 0x17, 0x39, 0xC0, 0x5C, 0x8F, 0xC2,
            0xF5, 0x28, 0x4C, 0x41, 0x40, 0xEC, 0x51, 0xB8,
            0x1E, 0x85, 0x2B, 0x34, 0xC0, 0xD5, 0x78, 0xE9,
            0x26, 0x31, 0x68, 0x43, 0x40, 0x6F, 0x12, 0x83,
            0xC0, 0xCA, 0xD1, 0x41, 0xC0, 0x1B, 0x2F, 0xDD,
            0x24, 0x06, 0x01, 0x2B, 0x40, 0xA4, 0x70, 0x3D,
            0x0A, 0xD7, 0x93, 0x43, 0xC0, 0x54, 0xE3, 0xA5,
            0x9B, 0xC4, 0x60, 0x25, 0x40, 0x64, 0x3B, 0xDF,
            0x4F, 0x8D, 0x17, 0x39, 0xC0,
        };

        this->little_endian = {
            0x01,
            0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x40
        };

        this->big_endian = {
            0x00,
            0x00, 0x00, 0x00, 0x01,
            0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
    }

    void TearDown() override {}
    
    std::vector<wkb::byte_t> wkb;
    std::vector<wkb::byte_t> little_endian;
    std::vector<wkb::byte_t> big_endian;
};

TEST_F(WKB_Test, wkb_read_test)
{    
    const wkb::geometry geom = wkb::read_wkb(this->wkb);
    EXPECT_EQ(geom.type, 3);

    const wkb::polygon& poly = boost::get<wkb::polygon>(geom.data);
    const std::vector<std::pair<double, double>> expected_coordinates = {
        {10.689, -25.092},
        {34.595, -20.170},
        {38.814, -35.639},
        {13.502, -39.155},
        {10.689, -25.092}
    };

    for (int i = 0; i < expected_coordinates.size(); i++) {
        EXPECT_NEAR(poly.rings[0].points[i].x, expected_coordinates[i].first, 0.0001);
        EXPECT_NEAR(poly.rings[0].points[i].y, expected_coordinates[i].second, 0.0001);
    }

    const wkb::wkt wkt_visitor;
    const auto wkt = "POLYGON ((10.689 -25.092,34.595 -20.170,38.814 -35.639,13.502 -39.155,10.689 -25.092))";
    EXPECT_EQ(boost::apply_visitor(wkt_visitor, geom.data), wkt);
    EXPECT_EQ(wkt_visitor(poly), wkt);
}

TEST_F(WKB_Test, wkb_endianness_test)
{
    wkb::point geom_big    = wkb::read_known_wkb<wkb::point>(this->big_endian);
    wkb::point geom_little = wkb::read_known_wkb<wkb::point>(this->little_endian);
    EXPECT_NEAR(geom_big.x, geom_little.x, 0.000001);
    EXPECT_NEAR(geom_big.y, geom_little.y, 0.000001);
}