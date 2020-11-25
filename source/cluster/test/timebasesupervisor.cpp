#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

class TimeBaseSupervisorTest : public CppUnit::TestFixture {
public:
    void setUp() {
    }

    void tearDown() {
    }

    void testInitial() {
        CPPUNIT_FAIL("Not implemented yet");
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "TimeBaseSupervisorTest" } };

        suite->addTest( new CppUnit::TestCaller<TimeBaseSupervisorTest>{ "testInitial", &TimeBaseSupervisorTest::testInitial } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( TimeBaseSupervisorTest::suite() );

    return (runner.run("", false))?0:1;
}
