#include "Error_t.h"
#include <sstream>

namespace SVD {
namespace Tools {

Error_t::Error_t() noexcept
    : std::runtime_error(std::string()), m_Who(), m_What(),
      m_Type(Type_t::Fatal) {}

Error_t::Error_t(const Error_t::Type_t type, const std::string &who,
                 const std::string &what) noexcept
    : std::runtime_error(std::string()), m_Who(who), m_What(what),
      m_Type(type) {}

const char *Error_t::what() const noexcept {
  auto ss = std::stringstream();
  switch (m_Type) {
  case Type_t::Fatal:
    ss << "FATAL ERROR::";
    break;
  case Type_t::Warning:
    ss << "WARNING::";
    break;
  }

  ss << m_Who << "::" << m_What;
  return ss.str().c_str();
}

// Error_t::Error_t(const Error_t &rOther) noexcept
//    : std::runtime_error(rOther.m_What), m_Who(rOther.m_Who),
//      m_What(rOther.m_What), m_Msg(rOther.m_Msg), m_Type(rOther.m_Type) {}

// Error_t::Error_t(Error_t &&rOther) noexcept
//    : std::runtime_error(rOther.m_What), m_Who(std::move(rOther.m_Who)),
//      m_What(std::move(rOther.m_What)), m_Msg(std::move(rOther.m_Msg)),
//      m_Type(rOther.m_Type) {}

} // namespace Tools
} // namespace SVD
