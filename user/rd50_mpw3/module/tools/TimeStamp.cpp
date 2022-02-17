#include "TimeStamp.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace SVD {
namespace Tools {

std::mutex TimeStamp::m_gMutex;
std::string TimeStamp::Now(const std::string &rFormat) noexcept {
  auto ss = std::stringstream();
  TimeStamp::Now(ss, rFormat);
  return ss.str();
}

std::ostream &TimeStamp::Now(std::ostream &rStream,
                             const std::string &rFormat) noexcept {
  auto intime = std::time_t();
  {
    std::lock_guard<std::mutex> lock(m_gMutex);
    intime =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    rStream << std::put_time(std::localtime(&intime), rFormat.c_str());
  }
  return rStream;
}

} // namespace Tools
} // namespace SVD
