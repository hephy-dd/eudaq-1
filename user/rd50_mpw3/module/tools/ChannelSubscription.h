#pragma once

#include "cadef.h"

#include "../tools/ChannelAccess.h"
#include "../tools/ProcessVariable.h"

#include "../tools/Logger.h"

#include <iostream>
#include <mutex>

namespace SVD {
namespace Epics {

template <typename Task_t, int TypeIndex_t, PVStruct_t Type_t, size_t Size_t>
class ChannelSubscription {
private:
  using Error_t = Tools::Error_t;
  using Logger = Tools::Logger;
  static constexpr auto m_gName = "ChannelSubscription";
  static constexpr auto m_gConnectTimeout = ca_real{5};

  using PV_t = ProcessVariable<TypeIndex_t, Type_t, Size_t>;
  using PVData_t = typename PV_t::Value_t;
  static constexpr auto m_gIndex = PV_t::m_gTypeIndex;

public:
  static auto CallBack(event_handler_args args) noexcept -> void {
    using Sub_t = ChannelSubscription<Task_t, TypeIndex_t, Type_t, Size_t>;
    auto *const pMonitor = static_cast<Sub_t *const>(args.usr);

    if (nullptr == pMonitor) {
      Logger::Log(Logger::Type_t::Fatal, m_gName,
                  "Invalid channel subscription callback!");
      return void();
    } else if (Sub_t::m_gIndex != args.type) {
      auto ss = std::stringstream();
      ss << "PV Type missmatch! Expected " << Sub_t::m_gIndex << ", got "
         << args.type << '!';
      Logger::Log(Logger::Type_t::Warning, m_gName, ss.str());
      return void();
    } else if (ECA_NORMAL != args.status) {
      auto ss = std::stringstream();
      ss << "Failed to monitor PV! Got: " << ca_message(args.status);
      Logger::Log(Logger::Type_t::Warning, m_gName, ss.str());
      return void();
    }

    const auto *const pData = static_cast<const PVData_t *const>(args.dbr);
    if (nullptr == pData) {
      Logger::Log(Logger::Type_t::Warning, m_gName,
                  "Failed to convert dbr data!");
      return void();
    }

    // check if subscrition id is still valid...
    if (nullptr == pMonitor->m_ID)
      return void();

    pMonitor->m_PV.m_Value = *pData;
    try {
      pMonitor->m_Task(pMonitor->m_Access, pMonitor->m_PV);
    } catch (const Error_t &rError) {
      Logger::Log(rError);
    } catch (...) {
      Logger::Log(
          Logger::Type_t::Warning, m_gName,
          "Got an unexpected exception while performing a monitoring task!");
    }
  }

  ChannelSubscription() noexcept
      : m_PV(), m_Task(), m_PVName(), m_ID(nullptr), m_Access(false),
        m_Mask(0) {}

  ChannelSubscription(std::string &&rName, const uint64_t mask, Task_t &&rTask)
      : m_PV(), m_Task(std::move(rTask)), m_PVName(std::move(rName)),
        m_ID(nullptr), m_Access(), m_Mask(mask) {}

  ~ChannelSubscription() {
    if (nullptr != m_ID) {
      ca_clear_subscription(m_ID);
      m_ID = nullptr;
    }

    m_Access.Disconnect(m_PV);
    ca_pend_io(0);
  }

  inline auto Connect() {
    if (cs_conn == m_PV.GetConnectionState())
      return void();

    if (nullptr != m_ID)
      ca_clear_subscription(m_ID);
    m_ID = nullptr;

    m_Access.Connect(m_PVName, m_PV);
    auto status = m_Access.WaitForConnection(m_gConnectTimeout);

    if (ECA_NORMAL != status) {
      auto ss = std::stringstream();
      ss << "Failed to connect monitoring PV " << m_PVName
         << ". Got: " << ca_message(status);
      throw Error_t(Error_t::Type_t::Fatal, m_gName, ss.str());
    }

    // update PV value
    m_Access.BlockingPush(m_PV);

    // create subscription
    status = ca_create_subscription(
        PV_t::m_gTypeIndex, m_PV.GetSize(), m_PV.m_ID, m_Mask,
        static_cast<caEventCallBackFunc *const>(
            &ChannelSubscription<Task_t, TypeIndex_t, Type_t,
                                 Size_t>::CallBack),
        this, &m_ID);

    if (ECA_NORMAL != status) {
      auto ss = std::stringstream();
      ss << "Can not create subscription for " << m_PVName
         << ". Got: " << ca_message(status);
      throw Error_t(Error_t::Type_t::Fatal, m_gName, ss.str());
    }

    status = ca_pend_io(m_gConnectTimeout);
    if (ECA_NORMAL != status) {
      auto ss = std::stringstream();
      ss << "Can not create subscription for " << m_PVName
         << ". Got: " << ca_message(status);
      throw Error_t(Error_t::Type_t::Fatal, m_gName, ss.str());
    }
  }

  inline auto IsExitRequested() const noexcept {
    return m_Task.IsExitRequested();
  }

  inline auto GetConnectionState() const noexcept {
    return m_PV.GetConnectionState();
  }

private:
  ProcessVariable<TypeIndex_t, Type_t, Size_t> m_PV;
  Task_t m_Task;
  const std::string m_PVName;

  evid m_ID;
  ChannelAccess m_Access;

  const uint64_t m_Mask;
};

} // namespace Epics
} // namespace SVD
