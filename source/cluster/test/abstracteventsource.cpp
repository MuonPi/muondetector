#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

class AbstractEventSourceTest : public CppUnit::TestFixture {
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
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "AbstractEventSourceTest" } };

        suite->addTest( new CppUnit::TestCaller<AbstractEventSourceTest>{ "testInitial", &AbstractEventSourceTest::testInitial } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( AbstractEventSourceTest::suite() );

    return (runner.run("", false))?0:1;
}
