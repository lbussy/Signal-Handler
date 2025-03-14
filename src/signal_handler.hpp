/**
 * @file signal_handler.hpp
 * @brief Header file for the SignalHandler class.
 * @details Provides the SignalHandler class responsible for managing signal
 *          handling,including blocking signals, handling callbacks, and
 *          safely shutting down.
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

#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <unordered_map>

#include <termios.h>

/**
 * @enum SignalHandlerStatus
 * @brief Enum representing possible status codes for SignalHandler operations.
 */
enum class SignalHandlerStatus
{
    SUCCESS,           ///< Operation was successful.
    FAILURE,           ///< Operation failed.
    ALREADY_STOPPED,   ///< Signal handler was already stopped.
    PARTIALLY_BLOCKED, ///< Some signals could not be blocked.
    TIMEOUT            ///< Operation timed out.
};

/**
 * @class SignalHandler
 * @brief Manages signal handling and processing.
 * @details Provides mechanisms to block signals, register callbacks, and handle
 *          shutdowns safely within a multithreaded environment.
 */
class SignalHandler
{
public:
    using Callback = std::function<void(int, bool)>;

    /**
     * @brief Constructs a SignalHandler instance.
     * @param callback Optional callback function for signal handling.
     */
    explicit SignalHandler(Callback callback = {});

    /**
     * @brief Destructor to clean up resources.
     */
    ~SignalHandler();

    /**
     * @brief Requests a shutdown by setting the shutdown flag.
     * @return SignalHandlerStatus indicating the success or failure of the request.
     */
    SignalHandlerStatus request_shutdown();

    /**
     * @brief Waits for a shutdown signal.
     * @param timeout_seconds Optional timeout in seconds; defaults to no timeout.
     * @return SignalHandlerStatus indicating the outcome of the wait operation.
     */
    SignalHandlerStatus wait_for_shutdown(std::optional<int> timeout_seconds = std::nullopt);

    /**
     * @brief Sets the signal handling callback function.
     * @param callback The function to be invoked upon receiving a signal.
     * @return SignalHandlerStatus indicating success or failure of callback assignment.
     */
    SignalHandlerStatus set_callback(Callback callback);

    /**
     * @brief Blocks all signals specified in the signal map.
     * @return SignalHandlerStatus indicating success, failure, or partial success.
     */
    SignalHandlerStatus block_signals();

    /**
     * @brief Stops the signal handler and joins the signal-handling thread.
     * @return SignalHandlerStatus indicating success or failure of stopping the handler.
     */
    SignalHandlerStatus stop();

    /**
     * @brief Converts a signal number to its string representation.
     * @param signum The signal number.
     * @return The string representation of the signal.
     */
    static std::string_view signal_to_string(int signum);

private:
    static const std::unordered_map<int, std::pair<std::string_view, bool>> signal_map; ///< Maps signals to names and criticality.
    struct termios original_tty;                                                        ///< Stores terminal settings for restoration.

    std::atomic<bool> shutdown_flag; ///< Flag to indicate shutdown status.
    std::atomic<bool> tty_saved;     ///< Flag to indicate if terminal settings were saved.
    std::mutex callback_mutex;       ///< Mutex for thread-safe callback updates.
    Callback user_callback;          ///< Stores the signal callback function.
    std::thread signal_thread;       ///< Thread for signal handling.
    std::mutex cv_mutex;             ///< Mutex for condition variable synchronization.
    std::condition_variable signal_cv;      ///< Condition variable for shutdown handling.

    /**
     * @brief Handles received signals.
     * @param signum The signal number.
     */
    void signal_handler(int signum);

    /**
     * @brief Runs the signal-handling thread.
     */
    void signal_handler_thread();

    /**
     * @brief Restores terminal settings upon exit.
     * @return SignalHandlerStatus indicating success or failure of restoration.
     */
    SignalHandlerStatus restore_terminal_settings();

    /**
     * @brief Suppresses terminal signals like echo mode.
     */
    void suppress_terminal_signals();
};

/**
 * @brief Global unique pointer to a SignalHandler instance.
 */
extern std::unique_ptr<SignalHandler> handler;

#endif // SIGNAL_HANDLER_HPP