#pragma once

#include <mutex>

namespace SVD {
namespace Tools {

/**
 * @brief The TimeStamp class was introduces to have an work around for the
 * not-thread-safe std::localtime implementation...
 */
class TimeStamp {
public:
  /**
   * @brief Now returns the system time with the given format string. the format
   * is specified by `std::put_time`. Internally it calls `Now(std::ostream
   * &rStream, const std::string &rFormat)`, that provides a locking mechanism
   * for `std::chrono::system_clock::now()`, that is not thread safe.
   * @param rFormat specified by `std::put_time`
   * @return time stamp with given formatting.
   */
  static auto Now(const std::string &rFormat) noexcept -> std::string;

  /**
   * @brief Now streams the time stamp with the given formatting to the
   * provided `std::ostream` object. The format string is specified by
   * `std::put_time`. Before acquiring the time with
   * `std::chrono::system_clock::now()`, it requires locking a the internal
   * static mutex m_gMutex.
   * @param rStream object based on std::ostream
   * @param rFormat specified by `std::put_time`
   * @return ref to std::ostream object for chaining.
   */
  static auto Now(std::ostream &rStream, const std::string &rFormat) noexcept
      -> std::ostream &;

private:
  static std::mutex m_gMutex; /**< mutex protecting  */
};

} // namespace Tools
} // namespace SVD
