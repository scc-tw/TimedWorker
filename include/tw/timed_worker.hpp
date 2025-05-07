#ifndef TW_TIMED_WORKER_HPP
#define TW_TIMED_WORKER_HPP
#pragma once

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <stop_token>
#include <atomic>
#include <future>
#include <thread>
#include <cstring>

namespace tw
{
    template <class LogS = std::ostream, class F, class... Args>
    auto make_timed_worker(std::chrono::milliseconds timeout, F &&f, LogS &ls = std::cerr, Args &&...args);

    template <class LogStream = std::ostream>
    class TimedWorker
    {
    public:
        template <class LogS, class F, class... Args>
        friend auto make_timed_worker(std::chrono::milliseconds timeout, F &&f, LogS &ls, Args &&...args);

        void request_stop() noexcept { _thr.request_stop(); }
        void emergency_stop() noexcept { _emergency.store(true, std::memory_order_relaxed); }

        bool done() const noexcept { return _done.load(std::memory_order_acquire); }
        bool detached() const noexcept { return _detached; }

        TimedWorker(TimedWorker &&) noexcept = default;
        TimedWorker &operator=(TimedWorker &&) noexcept = default;
        TimedWorker(const TimedWorker &) = delete;
        TimedWorker &operator=(const TimedWorker &) = delete;

        ~TimedWorker()
        {
            if (_thr.joinable())
            {
                if (_done.load(std::memory_order_acquire))
                {
                    _thr.join();
                    return;
                }

                auto now = Clock::now();
                auto deadline = std::min(now + _timeout, _absDeadline);

                _thr.request_stop();
                if (_emergency.load(std::memory_order_relaxed))
                    deadline = now;

                auto fut = std::async(std::launch::async, [thr = std::move(_thr)]() mutable
                                      { thr.join(); });

                if (fut.wait_until(deadline) != std::future_status::ready)
                {
                    try
                    {
                        _log << "[TimedWorker] FORCED detach - resources may leak\n";
                    }
                    catch (...)
                    {
                    }
                    detach();
                }
            }
        }

    private:
        using Clock = std::chrono::steady_clock;

        template <class F>
        TimedWorker(std::chrono::milliseconds to, F &&f, LogStream &log = std::cerr)
            : _timeout(to), _absDeadline(Clock::now() + to), _log(log), _thr([this, func = std::forward<F>(f)](std::stop_token st) mutable
                                                                             {
              // register emergency flag as an additional stop condition
              std::stop_callback cb(st, [this]() noexcept {
                  _emergency.store(true, std::memory_order_relaxed);
              });

              // Skip work if stop was already requested
              if (!st.stop_requested())
              {
                  try { func(st); }
                  catch (std::exception const& ex) {
                      _log << "[TimedWorker] unhandled exception: " << ex.what() << '\n';
                  } catch (...) {
                      _log << "[TimedWorker] unknown exception\n";
                  }
              }

              _done.store(true, std::memory_order_release); })
        {
        }

        void detach() noexcept
        {
            _detached = true;
            std::jthread temp;
            std::memcpy((void *)&_thr, (void *)&temp, sizeof(temp));
        }

        std::chrono::milliseconds _timeout;
        Clock::time_point _absDeadline;
        std::jthread _thr;
        std::atomic_bool _done{false};
        std::atomic_bool _emergency{false};
        bool _detached{false};
        LogStream &_log;
    };

    template <class LogS, class F, class... Args>
    auto make_timed_worker(std::chrono::milliseconds timeout,
                           F &&f, LogS &ls, Args &&...args)
    {
        auto bound = [func = std::forward<F>(f),
                      tup = std::make_tuple(std::forward<Args>(args)...)](std::stop_token st) mutable
        {
            std::apply([&](auto &&...cur)
                       { func(st, std::forward<decltype(cur)>(cur)...); }, tup);
        };
        return TimedWorker<LogS>(timeout, std::move(bound), ls);
    }

} // namespace tw

#endif // TW_TIMED_WORKER_HPP
