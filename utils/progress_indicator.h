#ifndef UTILS_PROGRESS_INDICATOR_H
#define UTILS_PROGRESS_INDICATOR_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace wd_migrate::utils {
template <std::uint64_t update_iterations> struct iteration_update_policy {
  static auto should_update(std::uint64_t iterations) {
    return iterations % update_iterations == 0;
  }
};

template <typename update_policy = iteration_update_policy<1000>>
struct progress_indicator {
public:
  progress_indicator(const std::string &label) : label_(label) {}
  auto start() -> void {
    start_ = std::chrono::steady_clock::now();
    print_progress();
  }
  auto update() -> void {
    ++iterations_;
    if (update_policy::should_update(iterations_)) {
      print_progress();
    }
  }
  auto done() -> void {
    std::cout << label_ << " took ";
    print_time_millis(elapsed_millis());
    std::cout << std::string(20, ' ') << std::endl;
  }

private:
  auto print_progress() -> void {
    std::uint64_t it_per_second =
        1000.0 * (double)iterations_ / elapsed_millis();
    std::cout << "| " << label_ << ": " << iterations_ << " it "
              << it_per_second << " it/s "
              << "|" << std::string(20, ' ') << "\r";
    std::cout.flush();
  }
  auto elapsed_millis() -> std::uint64_t {
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_)
        .count();
  }
  void print_time_millis(std::uint64_t milliseconds) {
    const std::uint64_t hours = milliseconds / (1000 * 60 * 60);
    milliseconds %= (1000 * 60 * 60);
    const std::uint64_t minutes = milliseconds / (1000 * 60);
    milliseconds %= (1000 * 60);
    const std::uint64_t seconds = milliseconds / 1000;
    milliseconds %= 1000;
    std::cout << std::setfill('0') << std::setw(2) << hours << ":"
              << std::setw(2) << minutes << ":" << std::setw(2) << seconds
              << ":" << std::setw(3) << milliseconds;
  }

  const std::string label_;
  std::uint64_t iterations_ = 0;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};
} // namespace wd_migrate::utils

#endif // !UTILS_PROGRESS_INDICATOR_H
