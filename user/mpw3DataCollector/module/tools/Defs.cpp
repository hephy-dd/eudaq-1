#include "../tools/Defs.h"
#include <iomanip>

namespace SVD {
namespace Defs {

std::ostream &operator<<(std::ostream &rStream, RunMode_t runMode) noexcept {
  switch (runMode) {
  case RunMode_t::ADCDelay:
    rStream << "ADCDelay";
    break;
  case RunMode_t::Calibration:
    rStream << "Calibration";
    break;
  case RunMode_t::FIR:
    rStream << "FIR";
    break;
  case RunMode_t::Trigger:
    rStream << "Hardware";
    break;
  case RunMode_t::LocalTrigger:
    rStream << "LocalHardware";
    break;
  case RunMode_t::Noise:
    rStream << "Noise";
    break;
  case RunMode_t::Pedestal:
    rStream << "Pedestal";
    break;
  case RunMode_t::Sixlet:
    rStream << "Sixlet";
    break;
  case RunMode_t::VSep:
    rStream << "VSep";
    break;
  case RunMode_t::IV:
    rStream << "IV";
    break;
  }
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream, DataMode_t dataMode) noexcept {
  switch (dataMode) {
  case DataMode_t::Raw:
    rStream << "raw mode";
    break;
  case DataMode_t::TRP:
    rStream << "transparent mode";
    break;
  case DataMode_t::ZS:
    rStream << "zero-suppressed mode";
    break;
  case DataMode_t::ZSHT:
    rStream << "zero-suppressed hit time finding mode";
    break;
  }
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream, State_t state) noexcept {
  switch (state) {
  case State_t::Idle:
    rStream << "Idle";
    break;
  case State_t::Ready:
    rStream << "Ready";
    break;
  case State_t::Running:
    rStream << "Running";
    break;
  case State_t::Finished:
    rStream << "Finished";
    break;
  case State_t::Exited:
    rStream << "Exited";
    break;
  case State_t::Error:
    rStream << "Error";
    break;
  case State_t::Configuring:
    rStream << "Configuring";
    break;
  case State_t::Starting:
    rStream << "Starting";
    break;
  case State_t::Stopping:
    rStream << "Stopping";
    break;
  case State_t::Finishing:
    rStream << "Finishing";
    break;
  case State_t::Exiting:
    rStream << "Exiting";
    break;
  case State_t::Aborting:
    rStream << "Aborting";
    break;
  case State_t::Down:
    rStream << "Down";
    break;
  }
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream, Status_t status) noexcept {
  switch (status) {
  case Status_t::NotMonitored: {
    rStream << "NotMonitored";
  } break;
  case Status_t::Nominal: {
    rStream << "Nominal";
  } break;
  case Status_t::Warning: {
    rStream << "Warning";
  } break;
  case Status_t::Alarm: {
    rStream << "Alarm";
  } break;
  }

  return rStream;
}

std::string GetFileName(const std::string &rPath, const std::string &rPrefix,
                        RunMode_t mode, uint32_t exp, uint32_t run,
                        const std::string &rExt, uint32_t fileNr) noexcept {
  auto fname = std::stringstream();
  fname << rPath << '/' << rPrefix << '_' << mode << '_' << std::setw(2)
        << std::setfill('0') << exp << '-' << std::setw(6) << std::setfill('0')
        << run << '.' << rExt;

  if (0u != fileNr)
    fname << '.' << std::setw(3) << std::setfill('0') << fileNr;

  return fname.str();
}

std::ostream &operator<<(std::ostream &rStream, PSInhibit_t inhibit) noexcept {
  switch (inhibit) {
  case PSInhibit_t::Off: {
    rStream << "Off";
  } break;
  case PSInhibit_t::HV: {
    rStream << "HV";
  } break;
  case PSInhibit_t::LV: {
    rStream << "LV/HV";
  } break;
  }
  return rStream;
}

std::ostream &operator<<(std::ostream &rStream,
                         const Request_t &rRequest) noexcept {
  switch (rRequest) {
  case Request_t::Processed:
    rStream << "Processed";
    break;
  case Request_t::Configure:
    rStream << "Configure";
    break;
  case Request_t::Start:
    rStream << "Start";
    break;
  case Request_t::Stop:
    rStream << "Stop";
    break;
  case Request_t::Abort:
    rStream << "Abort";
    break;
  case Request_t::Exit:
    rStream << "Exit";
    break;
  }

  return rStream;
}

} // namespace Defs

namespace QM {
std::ostream &operator<<(std::ostream &rStream,
                         const FrontEndID_t &rID) noexcept {
  rStream << rID.GetLayer() << '.' << rID.GetLadder() << '.' << rID.GetSensor()
          << '.' << rID.GetSide();
  return rStream;
}

FrontEndID_t::FrontEndID_t(const uint8_t layer, const uint8_t ladder,
                           const uint8_t sensor, const uint8_t side) noexcept
    : m_ID((static_cast<uint32_t>(layer) << 24) |
           (static_cast<uint32_t>(ladder) << 16) |
           (static_cast<uint32_t>(sensor) << 8) | static_cast<uint32_t>(side)) {
}

} // namespace QM

namespace FADC_CTRL {
std::ostream &operator<<(std::ostream &rStream, const BackEndID_t id) noexcept {
  rStream << static_cast<int>(id.m_Crate) << '.'
          << static_cast<int>(id.m_Address);
  return rStream;
}
} // namespace FADC_CTRL
} // namespace SVD
