#pragma once

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <vector>

namespace SVD {
namespace Tools {

template <typename... Types_t> struct Storage_t;
template <typename... Types_t>
void swap(Storage_t<Types_t...> &rRHS, Storage_t<Types_t...> &rLHS) noexcept;

/**
 * @brief The Storage_t struct represents a class storing several type. The
 * unique identification is defined by the element type!
 */
template <typename... Types_t> struct Storage_t {
public:
  /**
   * @brief GetComponent<T> returns a reference to the component of type T. If T
   * is not in [Types_t...] it won't compile!
   */
  template <typename T> auto &GetComponent() noexcept {
    return std::get<T>(m_Components);
  }

  /**
   * @brief GetComponent<T> returns a const reference to the component of type
   * T. If T is not in [Types_t...] it won't compile!
   */
  template <typename T> auto &GetComponent() const noexcept {
    const auto &rComponent = std::get<T>(m_Components);
    return rComponent;
  }

  /**
   * @brief Swap swaps the registered T with the given T.
   * If T is not in [Types_t...] it won't compile!
   */
  template <typename T> inline auto Swap(T &rOther) noexcept {
    using std::swap;
    swap(GetComponent<T>(), rOther);
  }

  /**
   * @brief Swap swaps container itself. The TypeList [Types_t...] has to be
   * same for both storage types!
   */
  inline auto Swap(Storage_t<Types_t...> &rOther) noexcept {
    m_Components.swap(rOther.m_Components);
  }

  /**
   * @brief swap<> overload for swap function
   */
  friend auto swap<>(Storage_t<Types_t...> &rRHS,
                     Storage_t<Types_t...> &rLHS) noexcept -> void;

private:
  std::tuple<Types_t...>
      m_Components; /**< tuple containing compile time defined containers. */
};

/**
 * @brief swap<> overload for swap function.
 * default usage:
 * ```cpp
 * using std::swap;
 * swap(A, B)
 * ```
 */
template <typename... Types_t>
auto swap(Storage_t<Types_t...> &rRHS, Storage_t<Types_t...> &rLHS) noexcept
    -> void {
  rRHS.Swap(rLHS);
}

} // namespace Tools
} // namespace SVD
