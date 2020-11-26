#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <memory>
#include "coincidence.h"
#include "event.h"
#include "combinedevent.h"

class CombinedEventTest : public CppUnit::TestFixture {
private:

public:
    void setUp() {
    }

    void tearDown() {
    }

    void testEvent() {
        const auto now { std::chrono::steady_clock::now( )};
        auto event_1 { (std::make_unique<MuonPi::Event>(1, 1, now, std::chrono::nanoseconds{100}))};
        auto event_2 { std::make_unique<MuonPi::Event>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        auto event_3 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{140}, std::chrono::nanoseconds{100})};
        auto event_4 { std::make_unique<MuonPi::Event>(3, 1, now - std::chrono::microseconds{140}, std::chrono::nanoseconds{100})};
        auto combined { std::make_unique<MuonPi::CombinedEvent>(1, std::move(event_1))};
        CPPUNIT_ASSERT(now == combined->start());
        CPPUNIT_ASSERT(now == combined->end());
        combined->add_event(std::move(event_2));
        CPPUNIT_ASSERT(now == combined->start());
        CPPUNIT_ASSERT((now + std::chrono::microseconds(50)) == combined->end());
        combined->add_event(std::move(event_3));
        CPPUNIT_ASSERT(now == combined->start());
        CPPUNIT_ASSERT((now + std::chrono::microseconds(140)) == combined->end());
        combined->add_event(std::move(event_4));
        CPPUNIT_ASSERT((now - std::chrono::microseconds(140) )== combined->start());
        CPPUNIT_ASSERT((now + std::chrono::microseconds(140)) == combined->end());
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CombinedEventTest" } };

        suite->addTest( new CppUnit::TestCaller<CombinedEventTest>{ "testEvent", &CombinedEventTest::testEvent } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( CombinedEventTest::suite() );

    return (runner.run("", false))?0:1;
}
