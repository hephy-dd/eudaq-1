#pragma once

#include "cadef.h"

#include "../tools/Logger.h"

#include <array>
#include <string>

namespace SVD {
namespace Epics {

template <int TypeIndex> struct PVType_t;

/**
 * @brief Specialization on DBR_CHAR, a 8bit signed integral type.
 */
template <> struct PVType_t<DBR_CHAR> {
  static constexpr auto m_gValueIndex = DBR_CHAR;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_CHAR;
  static constexpr auto m_gTimeIndex = DBR_TIME_CHAR;

  using Value_t = dbr_char_t;
  using CTRL_t = dbr_ctrl_char;
  using Time_t = dbr_time_char;
};

/**
 * @brief Specialization on DBR_SHORT, a 16bit signed integral type.
 * @note DBR_INT is just a type alias to DBR_SHORT
 */
template <> struct PVType_t<DBR_SHORT> {
  static constexpr auto m_gValueIndex = DBR_SHORT;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_SHORT;
  static constexpr auto m_gTimeIndex = DBR_TIME_SHORT;

  using Value_t = dbr_short_t;
  using CTRL_t = dbr_ctrl_short;
  using Time_t = dbr_time_short;
};

/**
 * @brief Specialization on DBR_ENUM, a 16bit signed integral type.
 * @note dbr_enum_t is a enumeration type with max. of 16 states. The state
 * names can be queried with dbr_ctrl_enum.
 */
template <> struct PVType_t<DBR_ENUM> {
  static constexpr auto m_gValueIndex = DBR_ENUM;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_ENUM;
  static constexpr auto m_gTimeIndex = DBR_TIME_ENUM;

  using Value_t = dbr_enum_t;
  using CTRL_t = dbr_ctrl_enum;
  using Time_t = dbr_time_enum;
};

/**
 * @brief Specialization on DBR_LONG, a 32bit signed integral type.
 */
template <> struct PVType_t<DBR_LONG> {
  static constexpr auto m_gValueIndex = DBR_LONG;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_LONG;
  static constexpr auto m_gTimeIndex = DBR_TIME_LONG;

  using Value_t = dbr_long_t;
  using CTRL_t = dbr_ctrl_long;
  using Time_t = dbr_time_long;
};

/**
 * @brief Specialization on DBR_FLOAT, a 32bit floating type.
 */
template <> struct PVType_t<DBR_FLOAT> {
  static constexpr auto m_gValueIndex = DBR_FLOAT;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_FLOAT;
  static constexpr auto m_gTimeIndex = DBR_TIME_FLOAT;

  using Value_t = dbr_float_t;
  using CTRL_t = dbr_ctrl_float;
  using Time_t = dbr_time_float;
};

/**
 * @brief Specialization on DBR_DOUBLE, a 64bit floating type.
 */
template <> struct PVType_t<DBR_DOUBLE> {
  static constexpr auto m_gValueIndex = DBR_DOUBLE;
  static constexpr auto m_gCTRLIndex = DBR_CTRL_DOUBLE;
  static constexpr auto m_gTimeIndex = DBR_TIME_DOUBLE;

  using Value_t = dbr_double_t;
  using CTRL_t = dbr_ctrl_double;
  using Time_t = dbr_time_double;
};

/**
 * @brief The PVStruct_t enum specifies internal PV struct type.
 */
enum class PVStruct_t {
  Value = 0, /**< only PV value */
  CTRL = 1,  /**< a dbr_ctrl_<type> structure */
  Time = 2   /**< a dbr_time_<type> structure */
};

template <int TypeIndex_t, PVStruct_t Type_t> struct TypeChooser_t;

template <int TypeIndex_t>
struct TypeChooser_t<TypeIndex_t, PVStruct_t::Value> {
  static constexpr auto m_gIndex = PVType_t<TypeIndex_t>::m_gValueIndex;
  using Type_t = typename PVType_t<TypeIndex_t>::Value_t;
};

template <int TypeIndex_t> struct TypeChooser_t<TypeIndex_t, PVStruct_t::CTRL> {
  static constexpr auto m_gIndex = PVType_t<TypeIndex_t>::m_gCTRLIndex;
  using Type_t = typename PVType_t<TypeIndex_t>::CTRL_t;
};

template <int TypeIndex_t> struct TypeChooser_t<TypeIndex_t, PVStruct_t::Time> {
  static constexpr auto m_gIndex = PVType_t<TypeIndex_t>::m_gTimeIndex;
  using Type_t = typename PVType_t<TypeIndex_t>::Time_t;
};

/**
 * @brief The ProcessVariable class provides a `C++` interface for CA library.
 * This template instance specialized on PVs housing arrays.
 * @details The template parameters specifies the following:
 * - TypeIndex - type enum assigned by macro definitions, ie DBR_DOUBLE,
 * DBR_ENUM, etc.
 * - SubType_t - defines if its only value, value + control values and value +
 * time information
 * - Size  - number of array elements.
 */
template <int TypeIndex, PVStruct_t SubType_t, size_t Size>
class ProcessVariable {
public:
  using ElementType_t = typename TypeChooser_t<TypeIndex, SubType_t>::Type_t;
  using Value_t = std::array<ElementType_t, Size>;
  static constexpr auto m_gTypeIndex =
      TypeChooser_t<TypeIndex, SubType_t>::m_gIndex;

public:
  /**
   * @brief ProcessVariable initializes the object with it's internal storage to
   * 0.
   */
  ProcessVariable() noexcept : m_ID(nullptr), m_Value() {
    std::fill(m_Value.begin(), m_Value.end(), ElementType_t{0});
  }

  ProcessVariable(const ProcessVariable &rOther) = delete;
  ProcessVariable &operator=(const ProcessVariable &rOther) = delete;

  /**
   * @brief ProcessVariable moves the CA ID and value to this object.
   * @note the moved object is invalidated.
   * @param rOther object to by moved into this instance
   */
  ProcessVariable(ProcessVariable &&rOther) noexcept
      : m_ID(rOther.m_ID), m_Value(std::move(rOther.m_Value)) {
    rOther.m_ID = nullptr;
  }

  /**
   * @brief operator = implements the move assignment.
   * @param rOther object to by moved into this instance
   * @return new instance containing the value from the moved object
   */
  ProcessVariable &operator=(ProcessVariable &&rOther) noexcept {
    const auto state = m_ID != nullptr ? ca_state(m_ID) : cs_never_conn;

    if ((state != cs_never_conn) && (state != cs_closed) &&
        (state != cs_prev_conn)) {
      ca_clear_channel(m_ID);
      ca_pend_io(ca_real{0});
    }

    m_ID = rOther.m_ID;
    std::copy(rOther.m_Value.begin(), rOther.m_Value.end(), m_Value.begin());
    rOther.m_ID = nullptr;
  }

  /**
   * @brief GetConnectionState returns the connection state
   * @return channel_state enum defining connection state.
   */
  inline auto GetConnectionState() const noexcept {
    return m_ID == nullptr ? cs_never_conn : ca_state(m_ID);
  }

  /**
   * @brief GetName returns the name of the PV. If the PV is not connected the
   * returned string is empty!
   */
  inline auto GetName() const noexcept {
    return m_ID != nullptr ? std::string(ca_name(m_ID)) : std::string();
  }

  /**
   * @brief Set the internal storage with the given iterators
   * @param begin begin of data to be copied
   * @param end end of data to be copied. (last elem + 1!)
   */
  template <typename Iterator_t>
  inline auto Set(Iterator_t begin, Iterator_t end) noexcept {
    const auto elements = std::distance(begin, end);
    std::copy(begin,
              elements <= static_cast<const long>(Size) ? end : begin + Size,
              m_Value.begin());
  }

  /**
   * @brief Get returns a ref to std:array containing the values
   */
  inline auto Get() noexcept -> Value_t & { return m_Value; }

  /**
   * @brief Get returns a const ref to std:array containing the values
   */
  inline auto Get() const noexcept -> const Value_t & { return m_Value; }

  /**
   * @brief GetSize returns the size of the internal array also the size of the
   * waveform PV.
   */
  inline constexpr auto GetSize() const noexcept { return Size; }

private:
  friend class ChannelAccess;

  chid m_ID;       /**< channel id, handle to the ChannelAccess */
  Value_t m_Value; /**< internal waveform/PV array storage */
};

template <typename Task_t, int TypeIndex_t, PVStruct_t Type_t, size_t Size_t>
class ChannelSubscription;

/**
 * @brief The ProcessVariable<TypeIndex, SubType_t, 1> class implements a PV
 * housing only one value.
 */
template <int TypeIndex, PVStruct_t SubType_t>
class ProcessVariable<TypeIndex, SubType_t, 1> {
public:
  using ElementType_t = typename TypeChooser_t<TypeIndex, SubType_t>::Type_t;
  using Value_t = ElementType_t;
  static constexpr auto m_gTypeIndex =
      TypeChooser_t<TypeIndex, SubType_t>::m_gIndex;

public:
  /**
   * default ctor setting the handle and value to 0.
   */
  ProcessVariable() noexcept : m_ID(nullptr), m_Value(ElementType_t{0}) {}
  ProcessVariable(const ProcessVariable &rOther) = delete;
  ProcessVariable &operator=(const ProcessVariable &rOther) = delete;

  /**
   * move ctor
   */
  ProcessVariable(ProcessVariable &&rOther) noexcept
      : m_ID(rOther.m_ID), m_Value(std::move(rOther.m_Value)) {
    rOther.m_ID = nullptr;
  }

  /**
   * move assignment, disconnects the previous connection before the move.
   */
  ProcessVariable &operator=(ProcessVariable &&rOther) noexcept {
    const auto state = m_ID != nullptr ? ca_state(m_ID) : cs_never_conn;

    if ((state != cs_never_conn) && (state != cs_closed) &&
        (state != cs_prev_conn)) {
      ca_clear_channel(m_ID);
      ca_pend_io(ca_real{0});
    }

    m_ID = rOther.m_ID;
    m_Value = std::move(rOther.m_Value);
    rOther.m_ID = nullptr;

    return *this;
  }

  /**
   * dtor checking if PV is still connected.
   */
  ~ProcessVariable() {
    if (m_ID != nullptr) {
      const auto name = this->GetName();
      {
        const auto status = ca_clear_channel(m_ID);
        if (status != ECA_NORMAL)
          Tools::Logger::Log(Tools::Logger::Type_t::Fatal, name,
                             ca_message(status));
      }
      {
        const auto status = ca_pend_io(ca_real{0});
        if (status != ECA_NORMAL)
          Tools::Logger::Log(Tools::Logger::Type_t::Fatal, name,
                             ca_message(status));
      }
      m_ID = nullptr;
    }
  }

  /**
   * @brief GetConnectionState returns the connection state defines in
   * channel_state enum.
   */
  inline auto GetConnectionState() const noexcept {
    return m_ID == nullptr ? cs_never_conn : ca_state(m_ID);
  }

  /**
   * @brief GetName return the name of the connected PV. Empty string is not yet
   * connected.
   */
  inline auto GetName() const noexcept {
    return m_ID != nullptr ? std::string(ca_name(m_ID)) : std::string();
  }

  /**
   * @brief Set set the internal buffer to the given value
   * @param value value to be assigned
   */
  inline auto Set(const ElementType_t value) noexcept { m_Value = value; }

  /**
   * @brief GetSize is required by the interface definition and return the
   * number of values mapped to the PV name. For this specialization the return
   * value is always 1.
   */
  inline constexpr auto GetSize() const noexcept { return 1ul; }

  /**
   * @brief Get returns a copy of the value.
   */
  inline auto Get() noexcept { return m_Value; }

  /**
   * @brief Get returns a copy of the value.
   */
  inline auto Get() const noexcept { return m_Value; }

private:
  friend class ChannelAccess;

  template <typename Task_t, int TypeIndex_t, PVStruct_t Type_t, size_t Size_t>
  friend class ChannelSubscription;

  chid m_ID;             /**< channel id and handle to the ChannelAccess */
  ElementType_t m_Value; /**< locally buffered value */
};

} // namespace Epics
} // namespace SVD
