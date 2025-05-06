# ⏱️ TimedWorker

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)
[![CI Status](https://github.com/scc_tw/TimedWorker/workflows/CI/badge.svg)](https://github.com/scc_tw/TimedWorker/actions)

**A robust C++23 library for handling worker threads with safe termination guarantees**

</div>

## 🚀 Features

- **Header-only** - Just include and go, no linking required
- **Modern C++23** - Uses the latest C++ features like stop tokens and jthreads
- **Thread safety** - All operations are thread-safe
- **Termination guarantees** - Automatically handles unresponsive threads
- **Configurable timeouts** - Set how long to wait for worker termination
- **Customizable logging** - Inject your own logging implementation
- **Exception safety** - Proper handling of all exceptions
- **Signal-handler safe** - Emergency stop is async-signal-safe

## 📦 Requirements

| Compiler | Minimum Version | Notes |
|----------|-----------------|-------|
| GCC | 11+ | Full C++23 support in GCC 13+ |
| Clang | 13+ | Full C++23 support in Clang 17+ |
| AppleClang | 13.1+ | Recommended: 14+ on macOS 14 |
| MSVC | 19.44+ (VS 2022 17.4+) | With `/std:c++latest` flag |

- CMake 3.24 or higher
- Supports Linux, macOS, and Windows

## 📋 Quick Start

### Basic Example

```cpp
#include <tw/timed_worker.hpp>
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

int main() {
    // Create a worker that terminates itself if not responding within 500ms
    auto worker = tw::make_timed_worker(500ms, [](std::stop_token st) {
        std::cout << "Worker started\n";
        
        // Run until stop is requested or work is complete
        int count = 0;
        while (!st.stop_requested() && count < 5) {
            std::this_thread::sleep_for(100ms);
            std::cout << "Working... " << ++count << "\n";
        }
        
        std::cout << "Worker finished\n";
    });
    
    // Wait for worker to complete (a little longer than its execution time)
    std::this_thread::sleep_for(700ms);
    
    // Request stop (will automatically happen in destructor too)
    if (!worker.done()) {
        worker.request_stop();
    }
    
    return 0;
}
```

### Advanced Usage with Custom Logger

```cpp
#include <tw/timed_worker.hpp>
#include <sstream>
#include <fstream>

class FileLogger {
    std::ofstream file;
public:
    FileLogger(const std::string& path) : file(path) {}
    
    template<typename T>
    FileLogger& operator<<(const T& value) {
        file << value;
        return *this;
    }
};

int main() {
    FileLogger logger("worker.log");
    
    // Create a worker with custom logging
    auto worker = tw::make_timed_worker(1s, 
        [](std::stop_token st, int arg1, std::string arg2) {
            // Worker with additional parameters
            // Process arg1 and arg2...
        },
        logger,  // Custom logger instead of std::cerr
        42,      // arg1
        "hello"  // arg2
    );
    
    // Worker will safely terminate itself when destroyed
}
```

## 🔧 Building and Testing

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

## 📚 Integration

TimedWorker is designed to be easily integrated with your CMake projects:

### Option 1: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  timed_worker
  GIT_REPOSITORY https://github.com/scc-tw/TimedWorker.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(timed_worker)

# Link against the library
target_link_libraries(your_target PRIVATE timed_worker)
```

### Option 2: Install and find_package

```cmake
# After installing TimedWorker
find_package(timed_worker REQUIRED)
target_link_libraries(your_target PRIVATE tw::timed_worker)
```

## 🔍 Implementation Details

TimedWorker uses C++23's `std::jthread` and `std::stop_token` to manage the worker thread lifecycle. Key implementation features:

- **Cooperative cancellation** - Worker can check stop_requested() periodically
- **Timeout enforcement** - If worker doesn't respond within timeout, it's forcibly detached
- **Safe detach mechanism** - Prevents resource leaks and crashes
- **Exception handling** - All exceptions in the worker thread are caught and logged
- **Atomic operations** - Thread-safe status checking

## 🧪 Continuous Integration

The library is automatically tested on multiple platforms with compiler configurations that fully support C++23:

- **Linux (Ubuntu 24.04)**: GCC 13, Clang 17
- **macOS 14**: AppleClang 14
- **Windows 2022**: MSVC 19.44+

This ensures compatibility across all major platforms and C++23-compatible compilers.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👤 Author

Created and maintained by [@scc-tw](https://github.com/scc-tw).

## 🤝 Contributing

Contributions, issues and feature requests are welcome! Feel free to check the [issues page](https://github.com/scc-tw/TimedWorker/issues). 