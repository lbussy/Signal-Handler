#include "signal_handler.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unistd.h>

// Global unique instance of SignalHandler.
SignalHandler signalHandler;

// Variables to control program execution.
std::mutex cv_mutex;
std::condition_variable cv;
std::atomic<bool> stop_requested{false};

void signal_handler(int signum, bool critical = false)
{
    std::cout << "Caught signal " << SignalHandler::signalToString(signum) << ", stopping gracefully." << std::endl;
    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        stop_requested.store(true);
    }
    cv.notify_all(); // Wake up worker threads.
}

void worker_thread(int id)
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    while (!stop_requested.load())
    {
        // Simulate some work.
        for (volatile int i = 0; i < 1000000; ++i)
            ;
        // Wait for stop signal (non-busy waiting).
        cv.wait_for(lock, std::chrono::milliseconds(100), []
                    { return stop_requested.load(); });
    }
}

int main()
{
    // Block signals in main.
    block_signals();

    // Set up SignalHandler
    signalHandler.setPriority(10);
    signalHandler.setCallback(signal_handler);
    signalHandler.start();

    // Start worker threads to emulate other processing.
    std::vector<std::thread> workers;
    const int num_workers = 4;
    for (int i = 0; i < num_workers; ++i)
    {
        workers.emplace_back(worker_thread, i);
    }

    // Main thread waits for a termination signal.
    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv.wait(lock, []
                { return stop_requested.load(); });
    }

    // Stop the SignalHandler.
    signalHandler.stop();

    std::cout << "Waiting for worker threads to finish." << std::endl;
    for (auto &worker : workers)
    {
        worker.join();
    }

    std::cout << "All threads stopped. Exiting." << std::endl;
    return 0;
}
