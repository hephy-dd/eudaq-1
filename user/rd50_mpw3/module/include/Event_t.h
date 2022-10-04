#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "../tools/Defs.h"

namespace SVD {
namespace XLNX_CTRL {

/**
 * The Event_t struct contains data recorded in one SPY event.
 */
struct Event_t {
  uint32_t m_EventNr = Defs::VMEData_t{0}; /**< event number */
  uint32_t m_CTRL = Defs::VMEData_t{0};    /**< Controller status */
  uint32_t m_Words = Defs::VMEData_t{0};   /**< number of SPY data words */
  uint32_t m_FADC = Defs::VMEData_t{0};    /**< FADC status */
  uint32_t m_recvTs = 0;

  using Data_t = std::vector<Defs::VMEData_t>;
  std::vector<Data_t> m_Data; /**< SPY data container */
};

} // namespace XLNX_CTRL
} // namespace SVD
