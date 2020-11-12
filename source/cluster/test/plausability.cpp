#include "plausability.h"
#include "combinedevent.h"
#include "singleevent.h"

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <memory>

class PlausabilityTest : public CppUnit::TestFixture {
private:
    std::unique_ptr<MuonPi::Plausability> m_coincidence { nullptr };
public:
    void setUp() {
        m_coincidence = std::make_unique<MuonPi::Plausability>();
    }

    void tearDown() {
        m_coincidence.reset();
    }

    void testSingleEvents() {
        std::unique_ptr<MuonPi::AbstractEvent> first { std::make_unique<MuonPi::SingleEvent>(0, std::chrono::steady_clock::time_point{})};
        std::unique_ptr<MuonPi::AbstractEvent> second { std::make_unique<MuonPi::SingleEvent>(0, std::chrono::steady_clock::time_point{})};

        CPPUNIT_ASSERT(m_coincidence->criterion(first, second) == 0.0);
    }

    void testCombinedEvents() {
        std::unique_ptr<MuonPi::AbstractEvent> first { std::make_unique<MuonPi::CombinedEvent>(0)};
        std::unique_ptr<MuonPi::AbstractEvent> second { std::make_unique<MuonPi::CombinedEvent>(0)};

        CPPUNIT_ASSERT(m_coincidence->criterion(first, second) == 0.0);
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "PlausabilityTest" } };

        suite->addTest( new CppUnit::TestCaller<PlausabilityTest>{ "testSingleEvents", &PlausabilityTest::testSingleEvents } );
        suite->addTest( new CppUnit::TestCaller<PlausabilityTest>{ "testCombinedEvents", &PlausabilityTest::testCombinedEvents } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( PlausabilityTest::suite() );

    return (runner.run("", false))?0:1;
}
