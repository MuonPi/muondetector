#include "combinedevent.h"
#include "singleevent.h"

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <memory>

class CombinedEventTest : public CppUnit::TestFixture {
private:
    std::unique_ptr<MuonPi::CombinedEvent> m_combined_event { nullptr };
public:
    void setUp() {
        m_combined_event = std::make_unique<MuonPi::CombinedEvent>(0);
    }

    void tearDown() {
        m_combined_event.reset();
    }

    void testAddingEvents() {
        CPPUNIT_ASSERT(m_combined_event->n() == 0);
        CPPUNIT_ASSERT(m_combined_event->add_event(nullptr) == false);
        CPPUNIT_ASSERT(m_combined_event->n() == 0);

        std::unique_ptr<MuonPi::AbstractEvent> event { std::make_unique<MuonPi::SingleEvent>(0, std::chrono::steady_clock::time_point{})};
        CPPUNIT_ASSERT(m_combined_event->add_event(std::move(event)) == true);
        CPPUNIT_ASSERT(m_combined_event->n() == 1);
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CombinedEventTest" } };

        suite->addTest( new CppUnit::TestCaller<CombinedEventTest>{ "testAddingEvents", &CombinedEventTest::testAddingEvents } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( CombinedEventTest::suite() );

    return (runner.run("", false))?0:1;
}
