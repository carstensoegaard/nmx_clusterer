//
// Created by soegaard on 10/26/17.
//

#include <gtest/gtest.h>
#include <stdint.h>
//#include "test/TestBase.h"
#include "../NMXClustererSettings.h"
#include "../NMXClustererDefinitions.h"
#include "../NMXPlaneClusterer.h"

TEST(nmxClusterer__Test, testBitmask) {

    Clusterer c;

    EXPECT_FLOAT_EQ(std::pow(2, nmx::IGNORE_BITS), nmx::MAX_IGNORE);
    EXPECT_FLOAT_EQ(std::pow(2, nmx::MINOR_BITS), nmx::MAX_MINOR);
    EXPECT_FLOAT_EQ(std::pow(2, nmx::MAJOR_BITS), nmx::MAX_MAJOR);
}

TEST(nmxClusterer__Test, B1B2Test) {

    // Instanciate the event-builder
    Clusterer c;

    uint32_t time = 0x00000000;
    EXPECT_EQ(0, c.getB1(time));
    EXPECT_EQ(0, c.getB2(time));

    time = 0x00000020;
    EXPECT_EQ(1, c.getB1(time));
    EXPECT_EQ(0, c.getB2(time));

    time = 0x000000e0;
    EXPECT_EQ(7, c.getB1(time));
    EXPECT_EQ(0, c.getB2(time));

    time = 0x00ffffe0;
    EXPECT_EQ(7,     c.getB1(time));
    EXPECT_EQ(65535, c.getB2(time));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}