#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <memory>
#include "coincidence.h"
#include "abstractevent.h"
#include "singleevent.h"
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

    void testSingleEvent() {
        auto now { std::chrono::steady_clock::now( )};
        std::unique_ptr<MuonPi::AbstractEvent> event_1 { (std::make_unique<MuonPi::SingleEvent>(1, 1, now, std::chrono::nanoseconds{100}))};
        std::unique_ptr<MuonPi::AbstractEvent> event_2 { std::make_unique<MuonPi::SingleEvent>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> event_3 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{140}, std::chrono::nanoseconds{100})};

        CPPUNIT_ASSERT(m_coincidence->criterion(event_1, event_2));
        CPPUNIT_ASSERT(!m_coincidence->criterion(event_1, event_3));
        CPPUNIT_ASSERT(m_coincidence->criterion(event_2, event_3));
    }

    void testCombinedEvent() {
        auto now { std::chrono::steady_clock::now( )};
        std::unique_ptr<MuonPi::AbstractEvent> ev_1 { (std::make_unique<MuonPi::SingleEvent>(1, 1, now, std::chrono::nanoseconds{100}))};
        std::unique_ptr<MuonPi::AbstractEvent> ev_2 { std::make_unique<MuonPi::SingleEvent>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> ev_3 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{70}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> ev_4 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{80}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> ev_7 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{120}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> ev_8 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{90}, std::chrono::nanoseconds{100})};

        std::unique_ptr<MuonPi::CombinedEvent> ev_5 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_1))};
        if (!ev_5->add_event(std::move(ev_2))) {
            CPPUNIT_FAIL("Adding event failed.");
        }
        std::unique_ptr<MuonPi::CombinedEvent> ev_6 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_3))};
        if (!ev_6->add_event(std::move(ev_4))) {
            CPPUNIT_FAIL("Adding event failed.");
        }
        std::unique_ptr<MuonPi::CombinedEvent> ev_9 { std::make_unique<MuonPi::CombinedEvent>(1, std::move(ev_8))};
        if (!ev_6->add_event(std::move(ev_7))) {
            CPPUNIT_FAIL("Adding event failed.");
        }

        std::unique_ptr<MuonPi::AbstractEvent> single_1 { std::make_unique<MuonPi::SingleEvent>(2, 1, now + std::chrono::microseconds{50}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> single_2 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{70}, std::chrono::nanoseconds{100})};
        std::unique_ptr<MuonPi::AbstractEvent> combined_1 { std::move(ev_5) };
        std::unique_ptr<MuonPi::AbstractEvent> combined_2 { std::move(ev_6) };
        std::unique_ptr<MuonPi::AbstractEvent> combined_3 { std::move(ev_9) };
        std::unique_ptr<MuonPi::AbstractEvent> single_3 { std::make_unique<MuonPi::SingleEvent>(3, 1, now + std::chrono::microseconds{300}, std::chrono::nanoseconds{100})};

        CPPUNIT_ASSERT(m_coincidence->criterion(combined_1, single_1));
        CPPUNIT_ASSERT(m_coincidence->criterion(combined_1, combined_2));
        CPPUNIT_ASSERT(!m_coincidence->criterion(combined_1, single_3));
        CPPUNIT_ASSERT(!m_coincidence->criterion(combined_1, combined_3));

    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CoincidenceTest" } };

        suite->addTest( new CppUnit::TestCaller<CoincidenceTest>{ "testSingleEvent", &CoincidenceTest::testSingleEvent } );
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
