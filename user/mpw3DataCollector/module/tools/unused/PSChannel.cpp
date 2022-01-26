#include "PSChannel.h"
#include <chrono>
#include <cmath>
#include <thread>

namespace SVD {
namespace Tools {

PSChannel::PSChannel(ChannelAccess *const pAccess) noexcept
    : m_State(), m_Request(), m_VSet(), m_VConf(), m_pAccess(pAccess) {}

PSChannel::~PSChannel() {
  if (nullptr != m_pAccess) {
    m_pAccess->Disconnect(m_State);
    m_pAccess->Disconnect(m_Request);
    m_pAccess->Disconnect(m_VSet);
    m_pAccess->Disconnect(m_VConf);

    const auto status = ca_pend_io(ca_real{0});
    if (ECA_NORMAL != status)
      Logger::Log(
          Logger::Type_t::Fatal, m_gName,
          "Failed to disconnect PSChannel PVs, contact the developer! Got: " +
              std::string(ca_message(status)));
  }
}

void PSChannel::Connect(const std::string &rPrefix, const ca_real timeout) {
  m_pAccess->Connect(m_gStateName, m_State);
  m_pAccess->Connect(rPrefix + m_gRequestName, m_Request);
  m_pAccess->Connect(m_gVSetName, m_VSet);
  m_pAccess->Connect(m_gVConfName, m_VConf);

  const auto status = ca_pend_io(timeout);
  if (status != ECA_NORMAL) {
    auto ss = std::stringstream();
    ss << "Failed to connect to the following PVs:";
    if (m_State.GetConnectionState() != cs_conn)
      ss << ' ' << m_State.GetName();
    if (m_Request.GetConnectionState() != cs_conn)
      ss << ' ' << m_Request.GetName();
    if (m_VSet.GetConnectionState() != cs_conn)
      ss << ' ' << m_VSet.GetName();
    if (m_VConf.GetConnectionState() != cs_conn)
      ss << ' ' << m_VConf.GetName();
    throw Error_t(Error_t::Type_t::Fatal, m_gName, ss.str());
  }
}

void PSChannel::SetVoltage(const Defs::Float_t voltage) {
  if ((m_Request.GetConnectionState() != cs_conn) ||
      (m_VSet.GetConnectionState() != cs_conn))
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Lost connection to " + m_Request.GetName() + " or/and to " +
                      m_VSet.GetName() + "!");

  m_VSet.Set(static_cast<dbr_float_t>(voltage));
  m_pAccess->BlockingPush(m_VSet, ca_real{1}); // can throw

  m_Request.Set(static_cast<dbr_enum_t>(VRequest_t::SetVoltage));
  m_pAccess->BlockingPush(m_Request, ca_real{1}); // can throw
}

bool PSChannel::ConfirmVoltage(const Defs::Float_t voltage) {
  return (this->IsChannelReady() && this->IsRequestedVoltage(voltage));
}

void PSChannel::SetFinished() {
  if (m_Request.GetConnectionState() != cs_conn)
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Lost connection to " + m_Request.GetName() + "!");

  m_Request.Set(static_cast<dbr_enum_t>(VRequest_t::Finish));
  try {
    m_pAccess->BlockingPush(m_Request, ca_real{1}); // can throw
  } catch (const Error_t &rError) {
    Logger::Log(rError);
  }

  for (auto iRetry = 0; iRetry < 100;
       ++iRetry, std::this_thread::sleep_for(std::chrono::milliseconds(100)))
    try {
      m_pAccess->BlockingPull(m_Request, 1);
      if (m_Request.Get() == static_cast<dbr_enum_t>(VRequest_t::Processed))
        return void();
    } catch (const Error_t &rError) {
      Logger::Log(rError);
      if (m_Request.GetConnectionState() != cs_conn) {
        m_pAccess->Connect(m_Request.GetName(), m_Request);
        m_pAccess->WaitForConnection(5);
      }
    }
}

bool PSChannel::IsChannelReady() {
  if (m_State.GetConnectionState() != cs_conn)
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Lost connection to " + m_State.GetName() + "!");

  m_pAccess->BlockingPull(m_State, ca_real{1});
  if (m_State.Get() != static_cast<dbr_enum_t>(Defs::State_t::Running)) {
    auto ss = std::stringstream();
    ss << "PS is in state " << static_cast<Defs::State_t>(m_State.Get())
       << " instead of " << Defs::State_t::Running << '!';
    throw Error_t(Error_t::Type_t::Fatal, m_gName, ss.str());
  }

  if (m_Request.GetConnectionState() != cs_conn)
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Lost connection to " + m_Request.GetName() + "!");

  m_pAccess->BlockingPull(m_Request, ca_real{1});
  return m_Request.Get() == static_cast<dbr_enum_t>(VRequest_t::Processed);
}

bool PSChannel::IsRequestedVoltage(const Defs::Float_t voltage) {
  if (m_VConf.GetConnectionState() != cs_conn)
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Lost connection to " + m_VSet.GetName() + "!");

  m_pAccess->BlockingPull(m_VConf, ca_real{1});
  return std::abs(m_VConf.Get() - voltage) < Defs::Float_t{0.01};
}

} // namespace Tools
} // namespace SVD
