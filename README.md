# SignalHandler

## Overview

SignalHandler is a C++ library designed to manage signal handling in applications. It provides a structured way to intercept system signals, execute user-defined callbacks, and handle graceful shutdown procedures.

## Features

- Registers and blocks signals such as SIGINT, SIGTERM, SIGQUIT, and others.
- Allows custom callback functions for signal handling.
- Provides a thread-safe shutdown request mechanism.
- Ensures safe cleanup and resource management.

## Repository Structure

```text
├── LICENSE.md          # License information
├── .gitattributes      # Git attributes configuration
├── .gitignore          # Git ignore rules
└── src/
    ├── main.cpp        # Sample application
    ├── Makefile        # Build script
    ├── signal_handler.hpp # Header file for SignalHandler class
    ├── signal_handler.cpp # Implementation of SignalHandler
```

## Installation & Compilation

To build the project, navigate to the `src/` directory and run:

```bash
make
```

To clean up the compiled files:

```basg
make clean
```

## Usage

### Initializing the Signal Handler

In your `main.cpp`, instantiate the `SignalHandler` and set a callback function.  Here is an example implementation.  See `main.cpp` for a working demo.

```cpp
#include "signal_handler.hpp"

std::unique_ptr<SignalHandler> handler;

void custom_signal_handler(int signum, bool is_critical)
{
    if (is_critical)
    {
        std::cerr << "Critical signal received: " << signum << ". Exiting immediately." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        std::cout << "Signal received: " << signum << ". Initiating shutdown." << std::endl;
        handler->request_shutdown();
    }
}

int main()
{
    handler = std::make_unique<SignalHandler>();
    handler->set_callback(custom_signal_handler);
    handler->wait_for_shutdown();
    return 0;
}
```

### Handling Signals

By default, the SignalHandler manages these signals:

- `SIGINT`
- `SIGTERM`
- `SIGQUIT`
- `SIGHUP`
- `SIGSEGV` (critical)
- `SIGBUS` (critical)
- `SIGFPE` (critical)
- `SIGILL` (critical)
- `SIGABRT` (critical)

If a critical signal is received, the application will terminate immediately. Otherwise, a shutdown process is initiated.

## License

This project is licensed under the [MIT License](./LICENSE.md).

## Contributions

Contributions are welcome! If you find any issues or have suggestions for improvements, feel free to submit a pull request.

## Contact

For questions or support, please reach out via the repository's issue tracker.
