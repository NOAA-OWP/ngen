#include <vector>
#include <fstream>
#include <string>
#include <sstream>


#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


#include "gtest/gtest.h"

#include "utilities/StreamHandler.hpp"

class StreamOutputTest : public ::testing::Test {

    protected:

    StreamOutputTest() {

    }

    ~StreamOutputTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void StreamOutputTest::SetUp() {
    setupArbitraryExampleCase();
}

void StreamOutputTest::TearDown() {

}

//! Setup an arbitrary example case with essentially made-up values for the components, and place the components in the
/*!
 *  Setup an arbitrary example case with essentially made-up values for the components, and place the components in the
 *  appropriate member collections.
 */
void StreamOutputTest::setupArbitraryExampleCase() {

}

//! Test that the various StreamHandler Constructors Function.
TEST_F(StreamOutputTest, TestConstructors)
{


    ASSERT_TRUE(true);
}

TEST_F(StreamOutputTest, TestWithKnownInput)
{
    ASSERT_TRUE(true);
}



