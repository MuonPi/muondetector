#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>

#include <thread>

#include "ratesupervisor.h"


class RateSupervisorTest : public CppUnit::TestFixture {
public:
    void setUp() {
    }

    void tearDown() {
    }

    void testRate() {
        /*
        {
        MuonPi::RateSupervisor supervisor {{4.0f, 0.1f, 3.0f}};
        std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
        while (true) {
            std::size_t n {static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count())};
            if (n%250 == 0) {
                supervisor.tick(true);
            } else {
                supervisor.tick(false);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            if (n >= 10000) {
                break;
            }
        }
        CPPUNIT_ASSERT(std::abs(supervisor.current().s - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().n - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().m - 4.0f) < std::numeric_limits<float>::epsilon());
        }
        {
        MuonPi::RateSupervisor supervisor {{4.0f, 0.1f, 3.0f}};
        std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
        while (true) {
            std::size_t n {static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count())};
            if (n%500 == 0) {
                supervisor.tick(true);
            } else {
                supervisor.tick(false);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            if (n >= 10000) {
                break;
            }
        }
        CPPUNIT_ASSERT(std::abs(supervisor.current().s - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().n - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().m - 2.0f) < std::numeric_limits<float>::epsilon());
        }
        */
    }

    void testDeviation() {
        /*
        MuonPi::RateSupervisor supervisor {{4.0f, 0.1f, 3.0f}};
        std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
        std::size_t factor { 250 };
        while (true) {
            std::size_t n {static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count())};
            if (n%factor == 0) {
                supervisor.tick(true);
                factor += 50;
            } else {
                supervisor.tick(false);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            if (n >= 10000) {
                break;
            }
        }

        CPPUNIT_ASSERT(std::abs(supervisor.current().s - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().n - 0.0f) < std::numeric_limits<float>::epsilon());
        CPPUNIT_ASSERT(std::abs(supervisor.current().m - 4.0f) < std::numeric_limits<float>::epsilon());
        */
    }

    static auto suite() -> CppUnit::Test*
    {
        CppUnit::TestSuite* suite { new CppUnit::TestSuite{ "RateSupervisorTest" } };

        suite->addTest( new CppUnit::TestCaller<RateSupervisorTest>{ "testRate", &RateSupervisorTest::testRate } );
        suite->addTest( new CppUnit::TestCaller<RateSupervisorTest>{ "testDeviation", &RateSupervisorTest::testDeviation } );

        return suite;
    }
};


auto main() -> int
{
    CppUnit::TextUi::TestRunner runner {};
    runner.addTest( RateSupervisorTest::suite() );

    return (runner.run("", false))?0:1;
}
