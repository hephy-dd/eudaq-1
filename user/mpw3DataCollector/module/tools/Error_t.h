#pragma once

#include <stdexcept>

namespace SVD {
namespace Tools {

/**
 * @brief The Error_t struct defined the only allowed exception type the in SVD
 * FADC system.
 * @details The Error_t::Type_t indicated if it was a run breaking or
 * recoverable error.
 */
struct Error_t : std::runtime_error {
public:
  /**
   * @brief The Type_t enum defines the error type.
   */
  enum class Type_t : bool {
    Fatal,  /**< run breaking error */
    Warning /**< recoverable error */
  };

  /**
   * default ctor construction a empty string attributes and Type_t::Fatal.
   */
  Error_t() noexcept;

  /**
   * ctor constructed by copying the given strings and type
   * @param type severity type
   * @param who object issuing the error
   * @param what error MSG
   */
  Error_t(const Type_t type, const std::string &who,
          const std::string &what) noexcept;

  //---------------------------------------------------------------------------
  // defaulted operators
  //---------------------------------------------------------------------------
  Error_t(const Error_t &rOther) = default;
  Error_t(Error_t &&rOther) = default;
  virtual ~Error_t() final = default;

  /**
   * @brief what represents the override of std::exception
   * @return error MSG
   */
  auto what() const noexcept -> const char * final;

  /**
   * @brief GetType returns the error type
   * @see Error_t::Type_t
   */
  inline auto GetType() const noexcept { return m_Type; }

  /**
   * @brief GetWho returns the object issuing the exception
   * @return exception source
   */
  inline const auto &GetWho() const noexcept { return m_Who; }

  /**
   * @brief GetWhat returns the error MSG
   * @return error MSG
   */
  inline const auto &GetWhat() const noexcept { return m_What; }

private:
  std::string m_Who;   /**< object issuing the MSG */
  std::string m_What;  /**< log / error MSG */
  const Type_t m_Type; /**< error type */
};

} // namespace Tools
} // namespace SVD
