/**
 * @file main.cpp
 * @brief A sample application of the SignalHandler class.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "signal_handler.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

/// Global unique pointer to the SignalHandler instance.
std::atomic<bool> stop_requested_main{false};

/**
 * @brief Common shutdown request for signals or normal main() return.
 *
 * This function will request that the signal handler shut down cleanly.
 */
void handle_sig_shutdown()
{
    if (handler) // Ensure handler is valid
    {
        SignalHandlerStatus status = handler->request_shutdown();

        if (status == SignalHandlerStatus::ALREADY_STOPPED)
        {
            std::cerr << "[WARN] Shutdown already in progress. Ignoring duplicate request." << std::endl;
        }
        std::cerr << "[INFO] Shutdown requested." << std::endl;
    }
    else
    {
        std::cerr << "[ERROR] Handler is null. Cannot request shutdown." << std::endl;
    }
}

/**
 * @brief Waits for a keypress in anothter thread
 *
 * This funciton simulates a main program load and merely allows us to
 * simulate what happens when the program directs an exit and returns
 * to main().
 */
void keypress_listener()
{
    std::cout << "[INFO] Press any key to initiate shutdown." << std::endl;

    while (!stop_requested_main.load())
    {
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        fd_set set;
        struct timeval timeout = {0, 100000}; // 100ms
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, &timeout) > 0)
        {
            char c;
            read(STDIN_FILENO, &c, 1);
            stop_requested_main.store(true);
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[KEYPRESS] Simulated request to terminate program normally." << std::endl;
    handle_sig_shutdown();
}

/**
 * @brief Custom signal handling function.
 *
 * This function is called when a signal is received. It logs the signal and,
 * if critical, terminates immediately. Otherwise, it initiates a graceful shutdown.
 *
 * @param signum The signal number received.
 * @param is_critical Indicates whether the signal is critical.
 */
void custom_signal_handler(int signum, bool is_critical)
{
    std::string_view signal_name = SignalHandler::signal_to_string(signum);
    std::cout << "[DEBUG] Callback executed for signal " << signal_name << std::endl;

    if (is_critical)
    {
        std::cerr << "[ERROR] Critical signal received: " << signal_name << ". Performing immediate shutdown." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        std::cout << "[INFO] Intercepted signal: " << signal_name << ". Shutdown will proceed." << std::endl;

        handle_sig_shutdown();
    }
    // TODO: Here is where you signal other threads to start shutting down.
    stop_requested_main.store(true);
}

/**
 * @brief Main function.
 *
 * Initializes the SignalHandler instance, blocks signals, sets up the callback,
 * and waits for shutdown signals.
 *
 * @return int Returns 0 on successful shutdown, or 1 on failure.
 */
int main()
{
    handler = std::make_unique<SignalHandler>();

    if (handler->block_signals() != SignalHandlerStatus::SUCCESS)
    {
        std::cerr << "[ERROR] Failed to block signals. Exiting." << std::endl;
        return 1;
    }

    if (handler->set_callback(custom_signal_handler) != SignalHandlerStatus::SUCCESS)
    {
        std::cerr << "[ERROR] Failed to set signal callback." << std::endl;
        return 1;
    }

    // Start the keypress listener in a separate thread
    // This simulates the program going into another area to work
    std::thread keypress_thread(keypress_listener);
    //
    // Main thread waits for shutdown signal
    handler->wait_for_shutdown();
    // Wait for shutdown signal, times out after 5 seconds
    // if (handler->wait_for_shutdown(5) != SignalHandlerStatus::SUCCESS)
    // {
    //     std::cerr << "[ERROR] Shutdown process did not complete as expected." << std::endl;
    //     handler->request_shutdown();
    //     handler->wait_for_shutdown(1);
    // }

    // Cleanup
    if (keypress_thread.joinable())
    {
        keypress_thread.join();
    }

    handler->stop();
    handler.reset();

    return 0;
}
