#pragma once

#include <asio.hpp>
#include <iostream>
#include <chrono>
#include <functional>
using namespace std::chrono_literals;
using Clock = std::chrono::high_resolution_clock;
using Callback = std::function<void()>;
using asio::error_code;

// simple wrapper that makes it easier to repeat on fixed intervals
struct interval_timer {
    interval_timer(asio::io_context& io, Clock::duration i, Callback cb)
        : interval(i), callback(cb), timer(io)
    { run(); }

  private:
    void run() {
        timer.expires_after(interval);
        timer.async_wait([=](asio::error_code ec) {
            if (!ec) {
                callback();
                run();
            }
        });
    }

    Clock::duration const interval; 
    Callback callback;
    asio::high_resolution_timer timer;
};