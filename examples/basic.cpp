#include <tw/timed_worker.hpp>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

int main()
{
    std::cout << "Starting basic example...\n";

    // Create a worker with a 500ms timeout
    auto worker = tw::make_timed_worker(500ms, [](std::stop_token st)
                                        {
        std::cout << "Worker started\n";
        
        int count = 0;
        while (!st.stop_requested() && count < 5) {
            std::this_thread::sleep_for(100ms);
            std::cout << "Working... " << ++count << "\n";
        }
        
        std::cout << "Worker finished\n"; });

    // Wait for worker to complete (a little longer than its execution time)
    std::this_thread::sleep_for(700ms);

    // Worker should have naturally completed by now, but just in case
    if (!worker.done())
    {
        std::cout << "Explicitly requesting worker to stop...\n";
        worker.request_stop();
        // Give some time for the worker to stop gracefully
        std::this_thread::sleep_for(100ms);
    }

    std::cout << "Example completed\n";
    return 0;
}