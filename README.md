# ⏱️ TimedWorker

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)
![CI Status](https://github.com/scc-tw/TimedWorker/actions/workflows/ci.yml/badge.svg)

**A robust C++20 library for handling worker threads with safe termination guarantees**

</div>

## 🚀 Features

- **Header-only** - Just include and go, no linking required
- **Modern C++20** - Uses features like stop tokens and jthreads
- **Thread safety** - All operations are thread-safe
- **Termination guarantees** - Automatically handles unresponsive threads
- **Configurable timeouts** - Set how long to wait for worker termination
- **Customizable logging** - Inject your own logging implementation
- **Exception safety** - Proper handling of all exceptions
- **Signal-handler safe** - Emergency stop is async-signal-safe

## 📦 Requirements

| Compiler | Minimum Version | Notes |
|----------|-----------------|-------|
| GCC | 10+ | Full C++20 support in GCC 11+ |
| Clang | 13+ | Full C++20 support in Clang 13+ |
| AppleClang | **Not Supported** | AppleClang (even in Xcode 15) lacks `std::jthread` support |
| MSVC | 19.29+ (VS 2019 16.10+) | With `/std:c++latest` flag |

- CMake 3.24 or higher
- Supports Linux, macOS, and Windows
- For macOS users: Use LLVM Clang instead of AppleClang (install via Homebrew: `brew install llvm`)

## 📋 Quick Start

> ⚠️ **Important:** The worker function **must** accept a `std::stop_token` as its first parameter and regularly check `stop_requested()`. Infinite loops without stop checks are not supported and will be forcibly detached.

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
        while (!st.stop_requested() && count < 5) {  // <-- Always check stop_requested() in loops
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

TimedWorker uses C++20's `std::jthread` and `std::stop_token` to manage the worker thread lifecycle. Key implementation features:

- **Cooperative cancellation** - Worker can check stop_requested() periodically
- **Timeout enforcement** - If worker doesn't respond within timeout, it's forcibly detached
- **Safe detach mechanism** - Prevents resource leaks and crashes
- **Exception handling** - All exceptions in the worker thread are caught and logged
- **Atomic operations** - Thread-safe status checking

📝 **Best practice:** Always design your worker functions to periodically check `st.stop_requested()`, especially in loops or long-running operations. The library will forcibly detach threads that don't respond to stop requests within the specified timeout, which may lead to resource leaks.

## 🧪 Continuous Integration

The library is automatically tested on multiple platforms with compiler configurations that fully support C++20:

- **Linux (Ubuntu 24.04)**: GCC 13, Clang 18
- **macOS 14**: LLVM Clang 18 (not AppleClang, which lacks jthread support)
- **Windows 2022**: MSVC 19.44+

This ensures compatibility across all major platforms and C++20-compatible compilers.

## ⚠️ Known Issues

### AppleClang and jthread Compatibility

As of early 2024, AppleClang (including the version shipped with Xcode 15) does not fully support C++20 features required by TimedWorker:

- `std::jthread` is not implemented
- `std::stop_token` is not available

**Workarounds for macOS users:**

1. **Recommended:** Install LLVM Clang via Homebrew and use it instead of AppleClang:
   ```bash
   brew install llvm
   export CXX="$(brew --prefix llvm)/bin/clang++"
   export CXXFLAGS="-std=c++23"
   cmake -DCMAKE_CXX_COMPILER="$(brew --prefix llvm)/bin/clang++" ..
   ```

2. Use GCC instead:
   ```bash
   brew install gcc
   export CXX="$(brew --prefix gcc)/bin/g++-13"
   cmake -DCMAKE_CXX_COMPILER="$(brew --prefix gcc)/bin/g++-13" ..
   ```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions, issues and feature requests are welcome! Feel free to check the [issues page](https://github.com/scc-tw/TimedWorker/issues). 