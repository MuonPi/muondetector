#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

class CombinedEventTest : public CppUnit::TestFixture {
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
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CombinedEventTest" } };

        suite->addTest( new CppUnit::TestCaller<CombinedEventTest>{ "testInitial", &CombinedEventTest::testInitial } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( CombinedEventTest::suite() );

    return (runner.run("", false))?0:1;
}
