#pragma once

#include <cstdint>
#include <ostream>

#include "../tools/Error_t.h"

namespace SVD {
namespace Defs {

/**
 * @brief The EnvStatus enum defines the data status of the SVD system.
 */
enum class Status_t : uint8_t {
  NotMonitored = 0, /**< readout not running */
  Nominal = 1,      /**< system values are OK */
  Warning = 2,      /**< system values approaching alarm limit */
  Alarm = 3,        /**< system requires user interaction. */
};

/**
 * @brief operator << prints the EnvStatus status flags as string.
 * @param rStream ostream object
 * @param status status to be converted.
 * @return rStream the modified stream for chaining.
 */
auto operator<<(std::ostream &rStream, Status_t status) noexcept
    -> std::ostream &;

/**
 * @brief The PSInhibit_t enum indicates if certain channels are allowed to be
 * switched on or has to be shut down.
 */
enum class PSInhibit_t : uint8_t {
  Off = 0, /**< PS not inhibited */
  HV = 1,  /**< ramps down HV and VSep */
  LV = 2,  /**< ramps down HV VSep and in turn switches off LV */
};

/**
 * @brief operator << prints the PSInhibit_t as strings.
 * @param rStream given ostream object
 * @param inhibit PSInhibit_t
 * @return rStream the modified stream for chaining.
 */
auto operator<<(std::ostream &rStream, PSInhibit_t inhibit) noexcept
    -> std::ostream &;

/**
 * @brief The RunMode_t enum defines the allowed run modes!
 */
enum class RunMode_t : uint8_t {
  ADCDelay = 0, /**< ADC delay scan determines the delay25 setting. */
  FIR = 1,      /**< calculates the 8 FIR coefficients. */
  Pedestal = 2, /**< pedestal of each strip */
  Noise = 3,    /**< noise of each strip */
  Calibration =
      4,       /**< shaper pulse of each strip at a single injection level */
  Sixlet = 5,  /**< shaper pulse at several injection levels */
  Trigger = 6, /**< global trigger run */
  LocalTrigger =
      7, /**< local trigger run, used for test beams and standalone operation */
  VSep = 8, /**< shaper peak at different VSep settings. */
  IV = 9, /**< IV scan of each strip, (note only records monitoring values.) */
};

/**
 * @brief operator << is the manipulator overload for ostream objects. It prints
 * the RunMode_t as string instead of number.
 * @param rStream the given ostream object
 * @param runMode the run mode.
 * @return rStream the modified stream for chaining.
 */
std::ostream &operator<<(std::ostream &rStream, RunMode_t runMode) noexcept;

/**
 * @brief The State_t enum defines the external states for SVD:QM, SVD:FADC and
 * SVD:PS.
 */
enum class State_t : uint8_t {
  Idle = 0,     /**< system not configured */
  Ready = 2,    /**< system configured and started monitoring */
  Running = 4,  /**< system is taking data/monitoring/analyzing events */
  Finished = 6, /**< QM/FADC only, EndOfRun analysis performed successfully. */
  Exited = 8,   /**< the client exited IOC still running. */
  Error = 10,   /**< run breaking error occurred */
  Down = 12,    /**< not really used */

  Configuring = 1, /**< system still performing configuration */
  Starting = 3,    /**< system still evaluating the start request */
  Stopping = 5,    /**< system still trying to the stop request */
  Finishing = 7, /**< system still performing EndOfRun analysis, QM/FADC only */
  Exiting =
      9, /**< system performing the Exit request from the user over EPICS. */
  Aborting = 11, /**< system still aborting/ recovering to state Idle. */
};

/**
 * @brief operator << is the manipulator overload to stream the State_t as
 * string and not number.
 * @param rStream given std::ostream object
 * @param state is the State_t.
 * @return with modified content.
 */
std::ostream &operator<<(std::ostream &rStream, State_t state) noexcept;

/**
 * @brief The Request_t enum represents the user request to the SVD system.
 */
enum class Request_t : uint16_t {
  Processed = 0, /**< system has processed the previous request. */
  Configure = 1, /**< request to configure or switch LV on */
  Start = 2,     /**< request to start the run or ramp up HV/VSep */
  Stop = 3,      /**< request to stop the run or ramp down HV/VSep */
  Abort = 4, /**< request to reset the state state machine or switch off LV, HV
                and VSep */
  Exit = 5,  /**< request to close the client not and not the IOC. */
};

/**
 * @brief operator << is the manipulator overload, streaming user requests as
 * string instead of number.
 * @param rStream given std::ostream object
 * @param rRequest the request to be streamed.
 * @return a modified rStream.
 */
auto operator<<(std::ostream &rStream, const Request_t &rRequest) noexcept
    -> std::ostream &;

/**
 * @brief The DataMode_t enum defines the data mode acquired by FADC spy FIFO.
 * It determines the data mode flag on each FADC and the unpacker in the on the
 * QM side.
 */
enum class DataMode_t : uint8_t {
  Raw = 0x0,  /**< raw APV25 output */
  TRP = 0x1,  /**< reordered strip data, after FIR not ped. sub and cmc */
  ZS = 0x2,   /**< strip data over threshold given threshold */
  ZSHT = 0x3, /**< @todo hit time and max **NOT IMPLEMENTED YET** */
};

/**
 * @brief operator << is the manipulator overload, streaming data type as string
 * instead of number.
 * @param rStream given std::ostream object
 * @param dataMode the data mode to be streamed.
 * @return a modified rStream
 */
std::ostream &operator<<(std::ostream &rStream, DataMode_t dataMode) noexcept;

using Float_t = float;         /**< Floating point type used by QM */
using VMEData_t = uint32_t;    /**< VME data size/type */
using VMEAddress_t = uint32_t; /**< VME address type */

/**
 * @brief APV25MuxToStripOrder transforms the multiplexer output into APV25
 * channel order
 * @param pos multiplexer readout position
 * @return corresponding APV25 channel position
 */
inline constexpr auto APV25MuxToChannelOrder(const uint32_t pos) noexcept {
  return (32 * (pos % 4)) + 8 * static_cast<uint32_t>(pos / 4) -
         (31 * static_cast<uint32_t>(pos / 16));
}

/**
 * @brief The MaskFlags_t enum defines the bit positions in side the Mask_t
 * structure.
 * @note any extension here requires an update in the Mask_t::FromInt<T>(const
 * T&) to keep type safety.
 */
enum class MaskFlags_t : uint8_t {
  Noise = 0,         /**< strips masked by the noise run */
  Calibration = 1,   /**< strips masked by the calibration run */
  Occupancy = 2,     /**< strip masked an occupancy run - to be extended. */
  UserDefinded = 31, /**< user defined mask */
};

/**
 * @brief The Mask_t struct contains strip masking information in a binary
 * format. it also support some binary operations (`bit-wise or`, `bit-wise
 * and`) to query the mask state.
 */
struct Mask_t {
  using Value_t = uint32_t;

  /**
   * @brief FromInt turn a given integral type to a Mask_t. used to read input
   * data
   * @param rVal Mask_t with given input bits.
   * @note it only takes integral types, floating types are not allowed!
   * @throw Error_t if the given integer contains bits that are not defined in
   * MaskFlags_t, @see MaskFlags_t.
   */
  template <typename Int_t> static constexpr auto FromInt(const Int_t &rVal) {
    static_assert(std::is_integral<Int_t>::value,
                  "Invalid argument, expected integral type!");
    constexpr auto val =
        ~((Value_t{1} << static_cast<Value_t>(MaskFlags_t::Noise)) |
          (Value_t{1} << static_cast<Value_t>(MaskFlags_t::Calibration)) |
          (Value_t{1} << static_cast<Value_t>(MaskFlags_t::Occupancy)) |
          (Value_t{1} << static_cast<Value_t>(MaskFlags_t::UserDefinded)));
    if ((val & rVal) != Value_t{0}) {
      throw Tools::Error_t(Tools::Error_t::Type_t::Fatal, "Mask_t",
                           "Invalid input, trying to convert " +
                               std::to_string(rVal) + " to Mask_t!");
      //      throw std::runtime_error("Invalid input, trying to convert " +
      //                               std::to_string(rVal) + " to Mask_t!");
    }

    return Mask_t{static_cast<Value_t>(rVal)};
  }

  /**
   * @brief operator | enables bit-wise or with MaskFlags_t.
   * @param rFlag enum specifying the defect
   */
  inline auto operator|(const MaskFlags_t &rFlag) noexcept {
    m_Selection |= (1 << static_cast<Value_t>(rFlag));
    return *this;
  }

  /**
   * @brief operator & enables bit-wise and for Mask_t
   * @return new bit-wise and'ed Mask_t
   */
  friend inline auto operator&(const Mask_t &rLHS, const Mask_t &rRHS) noexcept
      -> Mask_t {
    return Mask_t{static_cast<Value_t>(rRHS.m_Selection & rLHS.m_Selection)};
  }

  /**
   * @brief operator | enables bit-wise or for Mask_t
   * @return new bit-wise or'ed Mask_t
   */
  friend inline auto operator|(const Mask_t &rLHS, const Mask_t &rRHS) noexcept
      -> Mask_t {
    return Mask_t{static_cast<Value_t>(rLHS.m_Selection | rRHS.m_Selection)};
  }

  /**
   * @brief operator ~ enables bit-wise negation on Mask_t to filter out
   * undefined state!
   * @return new bit-wise negated Mask_t
   * @note this function can break type safety! since the or'ed bits contains
   * flags not defined in MaskFlags_t! it will be removed in the next update.
   */
  inline auto operator~() noexcept -> Mask_t {
    m_Selection = ~m_Selection;
    return *this;
  }

  /**
   * @brief operator == enables equality comparison
   * @param rOther Mask_t to be compared to
   */
  inline auto operator==(const Mask_t &rOther) const noexcept {
    return rOther.m_Selection == m_Selection;
  }

  /**
   * @brief operator != enables inequality comparison
   * @param rOther Mask_t to be compared to
   */
  inline auto operator!=(const Mask_t &rOther) const noexcept {
    return rOther.m_Selection != m_Selection;
  }

  /**
   * Reset clear the set flags
   */
  inline auto Reset() noexcept { m_Selection = Value_t{0}; }

  /**
   * Set the bit defined by the given MaskFlags_t regardless of it
   * previous state.
   * @param rFlag enum specifying the defect
   */
  inline auto Set(const MaskFlags_t &rFlag) noexcept { return *this | rFlag; }

  /**
   * @brief Unset the bit defined by the given MaskFlags_t regardless of it
   * previous state.
   * @param rFlag enum specifying the defect
   */
  inline auto Unset(const MaskFlags_t &rFlag) noexcept {
    static_assert(sizeof(rFlag) <= sizeof(Value_t),
                  "Requires the size to be smaller than the value type!");
    m_Selection &= (~(Value_t{1} << static_cast<Value_t>(rFlag)));
    return *this;
  }

  /**
   * @brief IsEmpty returns true if no mask bits has been set, ie a good strip
   * @return true if strip is not masked
   */
  inline auto IsEmpty() const noexcept { return Value_t{0} == m_Selection; }

  /**
   * @brief AnyMask returns if any bits are set!
   * @return true if a strip has been masked.
   */
  inline auto AnyMask() const noexcept { return !this->IsEmpty(); }

  /**
   * @brief AnyOf checks if any of the bit defined by the given Mask_t is
   * defined in this instance.
   * @param rOther contains the query bits.
   * @return true if there are any bits set on both Mask_t
   */
  inline auto AnyOf(const Mask_t &rOther) const noexcept {
    return (m_Selection & rOther.m_Selection) == Value_t{0};
  }

  /**
   * @brief AllOf checks if the given mask is a subset of the defined
   * @param rOther
   * @return if all the masks bits are the same.
   */
  inline auto AllOf(const Mask_t &rOther) const noexcept {
    return (m_Selection & rOther.m_Selection) == rOther.m_Selection;
  }

  Value_t m_Selection = Value_t{0} /**< internal store object */;
};

/**
 * @brief GetFileName is the general function to create XML and data paths with
 * the full name for FADC and QM. the file name consists of:
 *
 * ```
 * [path]/[prefix]_[run type]_[experiment number]-[run number].[extension]
 * ```
 *
 * if a file number has been specified that is not `0`, to the above output a
 * `.[file number]`. will be attached
 *
 * @param rPath output path.
 * @param rPrefix file prefix
 * @param mode run type
 * @param exp experiment number
 * @param run run number
 * @param rExt file extension
 * @param fileNr file number, only used by FADC
 * @return full path to the new XML or data file.
 */
auto GetFileName(const std::string &rPath, const std::string &rPrefix,
                 RunMode_t mode, uint32_t exp, uint32_t run,
                 const std::string &rExt, uint32_t fileNr = 0) noexcept
    -> std::string;

} // namespace Defs

namespace Const {
static constexpr auto gFIRFilters = 8; /**< number of FIR filters */
static constexpr auto gAPV25Channels =
    128; /**< number of APV25 channels (strips9 */
static constexpr auto gAPV25MaxFrames =
    6; /**< max number of consecutive frames */
static constexpr auto gAPV25sPerHybrid = 6; /**< APV25s per hybrid */
static constexpr auto gHybridsPerFADC = 8;  /**< hybrids per FADC */
static constexpr auto gMaxRawDataSize =
    0x7ff; /**< max data points of FADC readout */
static constexpr auto gMaxDigitalThresholdValues = 0x3ff; /**<  */
static constexpr auto gDCDCs = 8;  /**< number of DC/DCs (V1.25 + V2.5) */
static constexpr auto gFADCs = 52; /**< max number of FADCs */
static constexpr auto gMinCMSection = 32; /**< min CM section size */
static constexpr auto gCalGroups =
    8; /**< APV25 strip groups receiving cal. pulse (16 strips) */
static constexpr auto gCalDelayElements =
    8; /**< max number of cal delays elements */
static constexpr auto gADCChannels =
    gHybridsPerFADC * gAPV25sPerHybrid; /**< number of ADC channel per FADC */
static constexpr auto gADCDelayChannels =
    gADCChannels; /**< number of delay 25 channels */
static constexpr auto gAPV25sPerFADC =
    gHybridsPerFADC * gAPV25sPerHybrid; /**< number of APV25s per FADC */
static constexpr auto gRawSpyFifoSize = 1024 * 48;
} // end namespace Const

namespace QM {
/**
 * @brief The FrontEndID_t struct defines a unique ID for each SVD hybrid in the
 * Belle2 detector. The ID is composed of:
 *  - layer
 *  - ladder
 *  - sensor
 *  - side (0 == P/U and 1 == N/V)
 *
 * Internally the 4 given identifier is merged into a 32bit integer:
 * ```cpp
 * ID = (side & 0xff) | ((sensor & 0xff) << 8) | ((ladder & 0xff) << 16) |
 *      ((layer & 0xff) << 24);
 * ```
 */
struct FrontEndID_t {
  uint32_t m_ID =
      0u; /**< unique id composed of layer, ladder sensor and side */

  /**
   * @brief FrontEndID_t default ctor
   * @param id hashed integer containing the layer, ladder, sensor and side
   * information
   */
  explicit FrontEndID_t(const uint32_t id = 0u) noexcept : m_ID(id) {}

  /**
   * @brief FrontEndID_t ctor constructs a valid unique ID.
   * @details The unique ID is composed/hashed with following:
   * ```cpp
   * ID = (side & 0xff) | ((sensor & 0xff) << 8) | ((ladder & 0xff) << 16) |
   *      ((layer & 0xff) << 24);
   * ```
   *
   * @param layer detector layer ranging from [3-6]
   * @param ladder ladder id
   * @param sensor sensor id, with 1 = FW and layer-1 = BW
   * @param side 0x0 = p and 0x1 = n
   */
  FrontEndID_t(const uint8_t layer, const uint8_t ladder, const uint8_t sensor,
               const uint8_t side) noexcept;

  /**
   * @brief GetLayer returns the layer number.
   */
  inline auto GetLayer() const noexcept { return (m_ID >> 24); }

  /**
   * @brief GetLadder returns the ladder number.
   */
  inline auto GetLadder() const noexcept { return (m_ID >> 16) & 0xff; }

  /**
   * @brief GetSensor returns the sensor number.
   */
  inline auto GetSensor() const noexcept { return (m_ID >> 8) & 0xff; }

  /**
   * @brief GetSide returns the side.
   */
  inline auto GetSide() const noexcept { return m_ID & 0xff; }

  /*****************************************************************************
   * Comparison operators for sorting and other std::algorithms
   ****************************************************************************/
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator<(const FrontEndID_t &rOther) const noexcept {
    return m_ID < rOther.m_ID;
  }

  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator>(const FrontEndID_t &rOther) const noexcept {
    return m_ID > rOther.m_ID;
  }

  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator==(const FrontEndID_t &rOther) const noexcept {
    return m_ID == rOther.m_ID;
  }

  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator!=(const FrontEndID_t &rOther) const noexcept {
    return m_ID != rOther.m_ID;
  }

  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator<=(const FrontEndID_t &rOther) const noexcept {
    return !(m_ID > rOther.m_ID);
  }

  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on hash created during construction, each hash
   * mapped uniquely to each back end object.
   */
  inline auto operator>=(const FrontEndID_t &rOther) const noexcept {
    return !(m_ID < rOther.m_ID);
  }
};

/**
 * @brief operator << turns the FrontEndID_t to the string format and forward it
 * to the `std::ostream` object. the formatting consist of the following:
 *
 * ```
 * [layer].[ladder].[sensor].[side 0:p/u, 1:n/v]
 * ```
 *
 * @param rStream an object based on std::ostream.
 * @param rID front end id specifying a hybrid.
 * @return a ref to the given std::ostream object.
 */
std::ostream &operator<<(std::ostream &rStream,
                         const FrontEndID_t &rID) noexcept;
} // end namespace QM

namespace FADC_CTRL {
/**
 * @brief The BackEndID_t struct defines a unique ID for each BackEnd object:
 *  - FADC board
 *  - FADC Controller board
 *
 * The ID is composed of the following:
 *  - Crate: VME crate id
 *  - Address: unique FADC VME address (only the first 8 most significant bits)
 */
struct BackEndID_t {
  BackEndID_t() noexcept = default;

  /**
   * @brief BackEndID_t is the ctor constructing a BackEndID_t based on the
   * given VME address and crate id.
   * @param crate crate id mapped to the VME output
   * @param address VME base address of the FADC.
   */
  BackEndID_t(const uint8_t crate, const uint8_t address) noexcept
      : m_Crate(crate), m_Address(address) {}

  uint8_t m_Crate = 0; /**< crate ID [0-3], that also maps to the VME channel */
  uint8_t m_Address = 0; /**< the 8 most significant bits of the VME address */

  /**
   * GetAddress returns the FADC ID shifted to its default location, ie
   * D[31:24].
   * @return FADC ID
   */
  inline auto GetAddress() const noexcept -> uint32_t {
    return (static_cast<uint32_t>(m_Address) << 24);
  }

  /**
   * @brief GetIDAsUInt32 returns the ID as a unique uint32_t.
   * @return unique ID composed of Crate and FADC IDs
   */
  inline auto GetIDAsUInt32() const noexcept {
    return (static_cast<uint32_t>(m_Crate) << 8) |
           (static_cast<uint32_t>(m_Address) & 0xff);
  }

  /*****************************************************************************
   * Comparison operators for sorting and other std::algorithms
   ****************************************************************************/
  /**
   * Comparison operation, to enable interfacing to certain STL interface.
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator<(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() < rOther.GetIDAsUInt32();
  }
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator>(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() > rOther.GetIDAsUInt32();
  }
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator==(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() == rOther.GetIDAsUInt32();
  }
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator!=(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() != rOther.GetIDAsUInt32();
  }
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator<=(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() <= rOther.GetIDAsUInt32();
  }
  /**
   * Comparison operation, to enable interfacing to certain STL interface
   * The comparison is based on BackEndID_t::GetIDAsUInt32 mapped uniquely to
   * each back end object.
   */
  inline auto operator>=(const BackEndID_t &rOther) const noexcept {
    return this->GetIDAsUInt32() >= rOther.GetIDAsUInt32();
  }
};

/**
 * @brief operator << turns the BackEndID_t to the string format and forward it
 * to the `std::ostream` object. the formatting consist of the following:
 *
 * ```
 * [crate].[VME base address]
 * ```
 *
 * @note the latest update prints the VME base address in `dec` and not `hex`
 * specified in the XML file.
 *
 * @param rStream an object based on std::ostream.
 * @param id specifying a back end object, FADC controller or FADC
 * @return a ref to the given std::ostream object.
 */
auto operator<<(std::ostream &rStream, const BackEndID_t id) noexcept
    -> std::ostream &;

} // namespace FADC_CTRL
} // namespace SVD
