#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

// Standard Libraries
#include <atomic>
#include <csignal>
#include <functional>
#include <string_view>
#include <thread>
#include <unordered_map>

// System libraries
#include <termios.h>

void block_signals();
class SignalHandler
{
public:
    // Status enum to deliver state if desired.
    enum class Status
    {
        OK,
        ERROR
    };

    // Default constructor and destructor.
    SignalHandler();
    ~SignalHandler();

    // Delete copy constructor and assignment operator.
    SignalHandler(const SignalHandler &) = delete;
    SignalHandler &operator=(const SignalHandler &) = delete;

    // Explicitly start the signal handling thread.
    void start();

    // Set a callback to be called when a signal is caught.
    // The callback receives the signal number and a bool "immediate"
    // which indicates if this signal requires an immediate shutdown.
    void setCallback(const std::function<void(int, bool)> &cb);

    // Stop the signal handling thread.
    // Returns false if already stopped.
    bool stop();

    // Set CPU priority for the signal handling thread.
    // Returns true if successful.
    bool setPriority(int priority);

    // Map of signals to intercept.
    // Each entry maps a signal number to a pair: the signal name and a bool indicating immediate shutdown.
    static const std::unordered_map<int, std::pair<std::string_view, bool>> signal_map;

    static std::string_view signalToString(int signum);

private:
    // The thread function that waits for signals.
    void run();

    std::thread worker_thread;
    std::atomic<bool> running;          ///< Only set when the run() loop fails
    std::atomic<bool> stop_requested; // used to signal the thread to stop waiting
    std::atomic<bool> stopping;       // indicates that stop() was called (to suppress callback on dummy signal)
    std::function<void(int, bool)> callback;

    // Terminal settings storage for STDIN (file descriptor 0)
    // so that we can disable printing control characters (like "^C")
    // and later restore them.
    termios original_termios;
    bool termios_saved;

    // Signal set used by sigwaitinfo.
    sigset_t signal_set;
};

#endif // SIGNAL_HANDLER_HPP