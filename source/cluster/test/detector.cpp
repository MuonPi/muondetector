#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

class DetectorTest : public CppUnit::TestFixture {
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
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "DetectorTest" } };

        suite->addTest( new CppUnit::TestCaller<DetectorTest>{ "testInitial", &DetectorTest::testInitial } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( DetectorTest::suite() );

    return (runner.run("", false))?0:1;
}
