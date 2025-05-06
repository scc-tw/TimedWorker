#include <gtest/gtest.h>
#include <tw/timed_worker.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <atomic>

TEST(TimedWorker, StopsWhenRequested)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;
    std::atomic_int counter{0};

    {
        auto w = tw::make_timed_worker(500ms, [&](std::stop_token st)
                                       {
                                           while (!st.stop_requested())
                                               ++counter; },
                                       sink); // custom logger

        // Give some time for the worker to increment the counter
        std::this_thread::sleep_for(10ms);

        w.request_stop();
    }

    EXPECT_GT(counter.load(), 0);
}