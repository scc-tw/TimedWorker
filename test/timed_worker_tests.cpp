#include <gtest/gtest.h>
#include <tw/timed_worker.hpp>
#include <chrono>
#include <thread>
#include <sstream>
#include <atomic>
#include <stdexcept>
#include <functional>

// Import the stop_token name for convenience
using std::stop_token;

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

TEST(TimedWorker, FinishesNaturally)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;
    std::atomic_bool flag{false};

    {
        auto w = tw::make_timed_worker(1s, [&](std::stop_token)
                                       { flag.store(true, std::memory_order_release); }, sink);
        // wait a short while for the worker to finish
        for (int i = 0; i < 10 && !w.done(); ++i)
            std::this_thread::sleep_for(1ms);

        EXPECT_TRUE(w.done());
        EXPECT_TRUE(flag.load(std::memory_order_acquire));
    }
}

// A helper that purposefully ignores stop requests so the destructor must detach
static void uncooperative_worker(std::stop_token st)
{
    // Will ignore stop requests for some time, but we add a safety timeout
    // so the test doesn't hang indefinitely if something goes wrong
    auto start = std::chrono::steady_clock::now();
    auto max_duration = std::chrono::seconds(5); // Safety bound

    // If stop_requested() is true OR max_duration is exceeded, exit the loop
    while (!st.stop_requested() &&
           std::chrono::steady_clock::now() - start < max_duration)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST(TimedWorker, ForcedDetachDueToTimeout)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;
    sink << "Test starting\n";

    {
        // Very small timeout so the destructor will hit the detach logic quickly
        sink << "Creating worker\n";
        // Use a very short timeout to trigger the detach path quickly
        auto w = tw::make_timed_worker(10ms, [&sink](std::stop_token st)
                                       {
            sink << "Worker thread started\n";
            // Purposely ignore stop requests for a short time, but have a safety exit
            auto start = std::chrono::steady_clock::now();
            while (!st.stop_requested() && 
                   std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            sink << "Worker exiting normally\n"; }, sink);

        // Add a short sleep so destructor detach gets triggered
        sink << "Sleeping before destructor\n";
        std::this_thread::sleep_for(50ms);
        sink << "About to destroy worker\n";
    }
    sink << "Worker destroyed\n";

    EXPECT_NE(sink.str().find("FORCED detach"), std::string::npos);
    std::cout << "Test log: " << sink.str() << std::endl;
}

TEST(TimedWorker, EmergencyStopTriggersImmediateDetach)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;
    sink << "Emergency test starting\n";

    {
        // Create a flag to track if the worker is blocked
        std::atomic_bool worker_blocked{false};

        // Inline the worker function with debug logging - now with block before checking the stop token
        auto w = tw::make_timed_worker(100ms, [&sink, &worker_blocked](std::stop_token st)
                                       {
            sink << "Emergency worker thread started\n";
            
            // Block the worker for a bit to ensure emergency_stop has time to take effect
            // but the worker doesn't have time to check the stop token before destruction
            worker_blocked = true;
            // Use a longer sleep to ensure the worker is still running when emergency_stop is called
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            worker_blocked = false;
            
            sink << "Emergency worker finishing block\n";
            
            // Normal worker logic follows
            if (!st.stop_requested()) {
                sink << "Emergency worker NOT stopped (unexpected)\n";
            } else {
                sink << "Emergency worker sees stop request\n";
            }
            
            sink << "Emergency worker exiting\n"; }, sink);

        // Give worker time to start blocking
        std::this_thread::sleep_for(10ms);

        // Verify the worker has started and is blocked
        EXPECT_TRUE(worker_blocked) << "Worker should be blocked at this point";

        // escalate immediately – destructor should not wait
        sink << "Calling emergency_stop()\n";
        w.emergency_stop();
        sink << "About to destroy emergency worker\n";

        // We're not sleeping here - we want the destructor to
        // run while the worker is still blocked
    }
    sink << "Emergency worker destroyed\n";

    // Check that forced detach happened
    EXPECT_NE(sink.str().find("FORCED detach"), std::string::npos)
        << "Emergency stop should have triggered a forced detach";

    std::cout << "Emergency test log: " << sink.str() << std::endl;
}

TEST(TimedWorker, LogsUnhandledException)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;

    {
        auto w = tw::make_timed_worker(100ms, [](std::stop_token)
                                       { throw std::runtime_error("oops"); }, sink);
        // Give some time for the worker to throw and finish
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_NE(sink.str().find("unhandled exception"), std::string::npos);
}

TEST(TimedWorker, DoneStateTest)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;

    // Test the done() indicator for different worker completion states
    std::atomic_bool task_completed = false;

    // First case: worker completes on its own
    {
        sink << "Testing normal completion:\n";
        auto w = tw::make_timed_worker(1s, [&](std::stop_token)
                                       {
            sink << "Task starting\n";
            // Quick task that finishes quickly
            std::this_thread::sleep_for(10ms);
            task_completed = true;
            sink << "Task completed\n"; }, sink);

        // Wait for task to complete
        for (int i = 0; i < 10 && !w.done(); ++i)
        {
            std::this_thread::sleep_for(5ms);
        }

        EXPECT_TRUE(w.done()) << "Worker should be done after task completes";
        EXPECT_TRUE(task_completed) << "Task should be marked as completed";
    }

    // Reset and test stopped case
    task_completed = false;
    sink.str(""); // Clear the log

    {
        sink << "Testing stop behavior:\n";
        auto w = tw::make_timed_worker(1s, [&](std::stop_token st)
                                       {
            sink << "Long task starting\n";
            // Keep checking stop token
            while (!st.stop_requested() && !task_completed) {
                std::this_thread::sleep_for(5ms);
            }
            sink << "Task stopped\n"; }, sink);

        // Give the task time to start
        std::this_thread::sleep_for(10ms);

        // Request stop
        w.request_stop();

        // Wait to confirm it completes
        for (int i = 0; i < 20 && !w.done(); ++i)
        {
            std::this_thread::sleep_for(5ms);
        }

        EXPECT_TRUE(w.done()) << "Worker should be done after stop";
        EXPECT_FALSE(task_completed) << "Task should NOT be completed after stop";
    }

    std::cout << "Done test log: " << sink.str() << std::endl;
}

TEST(TimedWorker, HandlesUnknownExceptions)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;

    {
        auto w = tw::make_timed_worker(100ms, [](std::stop_token)
                                       {
            // Throw something other than std::exception
            class UnknownException {};
            throw UnknownException{}; }, sink);

        // Give some time for the worker to throw and finish
        std::this_thread::sleep_for(10ms);
    }

    EXPECT_NE(sink.str().find("unknown exception"), std::string::npos);
}

TEST(TimedWorker, MoveWorkerToFunction)
{
    using namespace std::chrono_literals;
    std::ostringstream sink;
    std::atomic_bool task_running{false};
    std::atomic_bool task_completed{false};

    // Create a worker
    auto w1 = tw::make_timed_worker(500ms, [&](std::stop_token st)
                                    {
        task_running = true;
        sink << "Task started\n";
        
        // Run until requested to stop
        while (!st.stop_requested() && !task_completed) {
            std::this_thread::sleep_for(5ms);
        }
        
        sink << "Task finishing\n"; }, sink);

    // Give time for task to start
    std::this_thread::sleep_for(10ms);
    EXPECT_TRUE(task_running);

    // Create a function that accepts a TimedWorker by r-value reference (move)
    auto process_worker = [](tw::TimedWorker<std::ostringstream> &&worker)
    {
        // The worker is now in this function's scope
        worker.request_stop();
        // Return something to show we processed it
        return worker.done();
    };

    // Move the worker to the function
    process_worker(std::move(w1));

    // At this point w1 is moved-from and should not be used
    // Wait for task to report completion in the output
    std::this_thread::sleep_for(50ms);

    EXPECT_NE(sink.str().find("Task finishing"), std::string::npos);
}