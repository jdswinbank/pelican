#ifndef ADAPTERLOFARSTATIONVISIBILITIESTEST_H
#define ADAPTERLOFARSTATIONVISIBILITIESTEST_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * @file AdapterLofarStationVisibilitiesTest.h
 */

namespace pelican {

/**
 * @class AdapterLofarStationVisibilitiesTest
 *
 * @brief
 *
 * @details
 *
 */

class AdapterLofarStationVisibilitiesTest : public CppUnit::TestFixture
{
    private:
        friend class VisGen;
    public:
        CPPUNIT_TEST_SUITE( AdapterLofarStationVisibilitiesTest );
        CPPUNIT_TEST( test_method );
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp();
        void tearDown();

        // Test Methods
        void test_method();

    public:
        AdapterLofarStationVisibilitiesTest(  );
        ~AdapterLofarStationVisibilitiesTest();

    private:
};

} // namespace pelican
#endif // ADAPTERLOFARSTATIONVISIBILITIESTEST_H
