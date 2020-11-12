#include "coincidence.h"
#include "combinedevent.h"
#include "singleevent.h"

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <memory>

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

    void testSingleEvents() {
        std::unique_ptr<MuonPi::AbstractEvent> first { std::make_unique<MuonPi::SingleEvent>()};
        std::unique_ptr<MuonPi::AbstractEvent> second { std::make_unique<MuonPi::SingleEvent>()};

        CPPUNIT_ASSERT(m_coincidence->criterion(first, second) == false);
    }

    void testCombinedEvents() {
        std::unique_ptr<MuonPi::AbstractEvent> first { std::make_unique<MuonPi::CombinedEvent>()};
        std::unique_ptr<MuonPi::AbstractEvent> second { std::make_unique<MuonPi::CombinedEvent>()};

        CPPUNIT_ASSERT(m_coincidence->criterion(first, second) == false);
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "CoincidenceTest" } };

        suite->addTest( new CppUnit::TestCaller<CoincidenceTest>{ "testSingleEvents", &CoincidenceTest::testSingleEvents } );
        suite->addTest( new CppUnit::TestCaller<CoincidenceTest>{ "testCombinedEvents", &CoincidenceTest::testCombinedEvents } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( CoincidenceTest::suite() );

    return (runner.run("", false))?0:1;
}
