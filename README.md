# SignalHandler

## Overview

**SignalHandler** is a C++ library designed to manage POSIX signal handling in multi-threaded applications. It provides a dedicated thread to synchronously process signals, invoke custom callbacks, and enable structured and graceful shutdown procedures.

## Features

- Centralized handling of signals like `SIGINT`, `SIGTERM`, `SIGQUIT`, and others.
- Custom callback support with signal metadata (e.g., whether a signal is critical).
- Dedicated signal-handling thread to avoid race conditions.
- Thread-safe shutdown signaling using atomic flags and condition variables.
- Terminal configuration control (e.g., disable `^C` echo).
- Optional real-time priority adjustment for the signal-handling thread.

## Repository Structure

```text
├── LICENSE.md              # License information
├── .gitattributes          # Git attributes configuration
├── .gitignore              # Git ignore rules
└── src/
    ├── main.cpp            # Demonstration application using SignalHandler
    ├── Makefile            # Build script
    ├── signal_handler.hpp  # Header file for SignalHandler class
    ├── signal_handler.cpp  # Implementation of SignalHandler
```

## Installation & Compilation

To build the project, navigate to the `src/` directory and run:

```bash
make
```

To clean up the compiled files:

```bash
make clean
```

## Usage

### Initializing the Signal Handler

In your `main.cpp`, instantiate the `SignalHandler` and set a callback function.  Here is an example implementation.  See `main.cpp` for a working demo.

```cpp
#include "signal_handler.hpp"
#include <iostream>

SignalHandler signalHandler;

void custom_signal_handler(int signum, bool is_critical)
{
    std::cout << "Signal received: " << SignalHandler::signalToString(signum)
              << (is_critical ? " (critical)." : " (non-critical).") << std::endl;

    if (is_critical)
    {
        std::exit(EXIT_FAILURE);  // Immediate exit
    }
    // Set your application-specific shutdown flag here
}

int main()
{
    block_signals();  // Block signals in all threads
    signalHandler.setCallback(custom_signal_handler);
    signalHandler.start();
    signalHandler.setPriority(SCHED_RR, 10);  // Optional: requires CAP_SYS_NICE

    // Application logic here

    // On shutdown
    signalHandler.stop();
    return 0;
}
```

### Thread Integration

In multi-threaded applications, use condition variables or atomic flags to notify worker threads when a shutdown is requested, as demonstrated in main.cpp.

### Handling Signals

The following signals are registered and blocked by default, and marked as "critical" as noted:

| Signal | Immediate Exit |
| --- | --- |
| `SIGINT` | ❌ |
| `SIGTERM` | ❌ |
| `SIGQUIT` | ❌ |
| `SIGHUP` | ❌ |
| `SIGUSR1` | ❌ |
| `SIGSEGV` | ✅ |
| `SIGBUS` | ✅ |
| `SIGFPE` | ✅ |
| `SIGKILL` | ✅ |
| `SIGABRT` | ✅ |

If a critical signal is received and no callback is registered, the application will terminate immediately. Otherwise, the callback is responsible to properly handle the condition.

## Method Summary

- `void start()` – Starts the signal-handling thread.
- `void setCallback(const std::function<void(int, bool)>& cb)` – Registers a custom callback.
- `bool stop()` – Stops the thread and restores terminal settings.
- `bool setPriority(int policy, int priority)` – Sets scheduling policy and priority.
- `static std::string_view signalToString(int signum)` – Converts a signal to its name.

## License

This project is licensed under the [MIT License](./LICENSE.md).

## Contributions

Contributions are welcome! If you find any issues or have suggestions for improvements, feel free to submit a pull request.

## Contact

For questions or support, please reach out via the repository's issue tracker.
