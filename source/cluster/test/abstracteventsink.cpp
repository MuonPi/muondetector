#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

class AbstractEventSinkTest : public CppUnit::TestFixture {
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
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "AbstractEventSinkTest" } };

        suite->addTest( new CppUnit::TestCaller<AbstractEventSinkTest>{ "testInitial", &AbstractEventSinkTest::testInitial } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( AbstractEventSinkTest::suite() );

    return (runner.run("", false))?0:1;
}
