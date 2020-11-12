#include "singleevent.h"

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <memory>

class SingleEventTest : public CppUnit::TestFixture {
private:
    std::unique_ptr<MuonPi::SingleEvent> m_single_event { nullptr };
public:
    void setUp() {
        m_single_event = std::make_unique<MuonPi::SingleEvent>(0, std::chrono::steady_clock::time_point{});
    }

    void tearDown() {
        m_single_event.reset();
    }

    void testEvent() {
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "SingleEventTest" } };

        suite->addTest( new CppUnit::TestCaller<SingleEventTest>{ "testEvent", &SingleEventTest::testEvent } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( SingleEventTest::suite() );

    return (runner.run("", false))?0:1;
}
