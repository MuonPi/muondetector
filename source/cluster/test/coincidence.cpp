#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <memory>
#include "coincidence.h"
#include "event.h"
#include "combinedevent.h"

class CoincidenceTest : public CppUnit::TestFixture {
private:
    std::unique_ptr<MuonPi::Coincidence> m_coincidence { nullptr };

public:
    void setUp() {
        m_coincidence = std::make_unique<MuonPi::Coincidence>();
    }

    void tearDown() {
        m_coincidence.reset();
    }

    void testEvent() {
        auto now { std::chrono::steady_clock::now( )};
        auto event_1 { (std::make_unique<MuonPi::Event>(1, 1, now, std::chrono::nanoseconds{100}))};
        auto event_2 { std::make_unique<MuonPi::Event>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        auto event_3 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{140}, std::chrono::nanoseconds{100})};

        CPPUNIT_ASSERT(m_coincidence->criterion(event_1, event_2));
        CPPUNIT_ASSERT(!m_coincidence->criterion(event_1, event_3));
        CPPUNIT_ASSERT(m_coincidence->criterion(event_2, event_3));
    }

    void testCombinedEvent() {
        auto now { std::chrono::steady_clock::now( )};
        auto ev_1 { std::make_unique<MuonPi::Event>(1, 1, now, std::chrono::nanoseconds{100})};
        auto ev_2 { std::make_unique<MuonPi::Event>(2, 1, now + std::chrono::microseconds{10}, std::chrono::nanoseconds{100})};

        auto ev_3 { std::make_unique<MuonPi::Event>(3, 1, now - std::chrono::microseconds{12}, std::chrono::nanoseconds{100})};
        auto ev_4 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{14}, std::chrono::nanoseconds{100})};

        auto ev_7 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{120}, std::chrono::nanoseconds{100})};
        auto ev_8 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{90}, std::chrono::nanoseconds{100})};

        auto ev_5 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_1))};
        ev_5->add_event(std::move(ev_2));
        auto ev_6 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_3))};
        ev_6->add_event(std::move(ev_4));
        auto ev_9 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_8))};
        ev_9->add_event(std::move(ev_7));
        std::unique_ptr<MuonPi::Event> single_1 { std::make_unique<MuonPi::Event>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::Event> single_2 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{70}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::Event> combined_1 { std::move(ev_5) };
        std::unique_ptr<MuonPi::Event> combined_2 { std::move(ev_6) };
        std::unique_ptr<MuonPi::Event> combined_3 { std::move(ev_9) };
        std::unique_ptr<MuonPi::Event> single_3 { std::make_unique<MuonPi::Event>(3, 1, now + std::chrono::microseconds{300}, std::chrono::nanoseconds{100})};

        CPPUNIT_ASSERT(m_coincidence->criterion(combined_1, single_1));
        CPPUNIT_ASSERT(m_coincidence->criterion(combined_1, combined_2));
        CPPUNIT_ASSERT(!m_coincidence->criterion(combined_1, single_3));
        CPPUNIT_ASSERT(!m_coincidence->criterion(combined_1, combined_3));

    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CoincidenceTest" } };

        suite->addTest( new CppUnit::TestCaller<CoincidenceTest>{ "testEvent", &CoincidenceTest::testEvent } );
        suite->addTest( new CppUnit::TestCaller<CoincidenceTest>{ "testCombinedEvent", &CoincidenceTest::testCombinedEvent } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( CoincidenceTest::suite() );

    return (runner.run("", false))?0:1;
}
