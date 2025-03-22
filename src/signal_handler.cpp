// Project libraries
#include "signal_handler.hpp"

// Standard libraries

// System Libraries
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG_SIGNAL_HANDLER
#include <iostream> // Debug printing
#include <cstring>  // For strerror()
#endif

// Define the signal_map with the required signals.
const std::unordered_map<int, std::pair<std::string_view, bool>> SignalHandler::signal_map = {
    {SIGUSR1, {"SIGUSR1", false}},
    {SIGINT, {"SIGINT", false}},
    {SIGTERM, {"SIGTERM", false}},
    {SIGQUIT, {"SIGQUIT", false}},
    {SIGHUP, {"SIGHUP", false}},
    {SIGSEGV, {"SIGSEGV", true}},
    {SIGBUS, {"SIGBUS", true}},
    {SIGFPE, {"SIGFPE", true}},
    {SIGILL, {"SIGILL", true}},
    {SIGABRT, {"SIGABRT", true}}};

void block_signals()
    {
        sigset_t blockset;
        sigemptyset(&blockset);
        for (const auto &entry : SignalHandler::signal_map)
        {
            sigaddset(&blockset, entry.first);
        }
        if (pthread_sigmask(SIG_BLOCK, &blockset, nullptr) != 0)
        {
#ifdef DEBUG_SIGNAL_HANDLER
            perror("pthread_sigmask");
#endif
        }
    }

SignalHandler::SignalHandler()
    : running(true), stop_requested(false), stopping(false), termios_saved(false)
{
    // Save current terminal settings for STDIN (file descriptor 0).
    if (tcgetattr(STDIN_FILENO, &original_termios) == 0)
    {
        termios_saved = true;
        termios new_termios = original_termios;
#ifdef ECHOCTL
        new_termios.c_lflag &= ~ECHOCTL;
#endif
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    }

    // Initialize the signal set to include all signals from our signal_map.
    sigemptyset(&signal_set);
    for (const auto &entry : signal_map)
    {
        sigaddset(&signal_set, entry.first);
    }
    // Block these signals in the current thread (inheritable by subsequently spawned threads).
    if (pthread_sigmask(SIG_BLOCK, &signal_set, nullptr) != 0)
    {
#ifdef DEBUG_SIGNAL_HANDLER
        perror("pthread_sigmask");
#endif
    }
}

void SignalHandler::start()
{
    // Start the signal handling thread explicitly.
    worker_thread = std::thread(&SignalHandler::run, this);
}

SignalHandler::~SignalHandler()
{
    if (!running.load())
        return;
    else
        stop();
}

void SignalHandler::setCallback(const std::function<void(int, bool)> &cb)
{
    callback = cb;
}

std::string_view SignalHandler::signalToString(int signum) {
    auto it = signal_map.find(signum);
    if (it != signal_map.end()) {
        return it->second.first;
    }
    return "UNKNOWN";
}

bool SignalHandler::stop() {
    if (!running.load() || stop_requested.load())
        return false;
    stop_requested.store(true);
    // Send a dummy signal (using one from our set) to wake the thread if it is blocked.
    pthread_kill(worker_thread.native_handle(), SIGUSR1);
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    // Restore original terminal settings.
    if (termios_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    }
    return true;
}

bool SignalHandler::setPriority(int priority)
{
    // Use pthread_setschedparam to set CPU priority.
    sched_param sch_params;
    sch_params.sched_priority = priority;
    int ret = pthread_setschedparam(worker_thread.native_handle(), SCHED_FIFO, &sch_params);
    return (ret == 0);
}

void SignalHandler::run()
{
#ifdef DEBUG_SIGNAL_HANDLER
    std::cout << "Signal thread running, waiting for signals." << std::endl;
#endif

    // Build a local copy of the signal set from the static map.
    sigset_t local_set;
    sigemptyset(&local_set);
    for (const auto &entry : signal_map)
    {
        sigaddset(&local_set, entry.first);
    }

    while (running.load() && !stop_requested.load())
    {
        siginfo_t siginfo;
        int sig = sigwaitinfo(&local_set, &siginfo);

        // If shutdown has been requested, break immediately before calling callback.
        if (stop_requested.load())
            break;

        bool immediate = false;
        auto it = signal_map.find(sig);
        if (it != signal_map.end())
        {
            immediate = it->second.second;
        }

        // If a callback is set, call it.
        if (callback)
        {
#ifdef DEBUG_SIGNAL_HANDLER
            // TODO:  Get signal name
            std::cout << "Callback requested for signal: " << SignalHandler::signalToString(sig) << std::endl;
            std::cout << std::flush;
#endif
            callback(sig, immediate);
        }
        else
        {
            // If no callback is set, then obey the immediate flag by breaking.
            if (immediate) // TODO: Get and somehow kill the parent process?
                break;
        }
    }
    running.store(false);
}
