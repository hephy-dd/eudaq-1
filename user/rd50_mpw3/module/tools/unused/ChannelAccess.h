#pragma once

#include <atomic>
#include <iostream>
#include <string>

#include <cadef.h>

#include "../tools/Error_t.h"
#include "../tools/Logger.h"

namespace SVD {
namespace Epics {

/**
 * @brief The ChannelAccess class abstracts the C interface of `Channel Access
 * library`.
 * @details This class allows the user to create a handle in a new context. The
 * `Channel Access` library is used to connect to PVs hosted by their
 * corresponding IOCs. The read access is allowed for all PV fields. However
 * the write access only works on the PV value, while modifying alarm limits ans
 * severity is not allowed.
 */
class ChannelAccess {
public:
  /**
   * @brief ChannelAccess constructs the object w/ or w/o preemptive context.
   * @param preemptive en/disable preemptive context
   * @throw Tools::Error_t if the context construction fails.
   */
  ChannelAccess(const bool preemptive = true);

  /**
   * The dtor check if the any outstanding pushes are left and block for
   * max. 30 seconds destructing the handle anyway. In that specific case an
   * error message is logged.
   */
  ~ChannelAccess();

  /*****************************************************************************
   * PV connections
   ****************************************************************************/
  /**
   * @brief Connect connected the given Epics::ProcessVariable with the
   * specified name.
   * @details The channel upon an successful connection is moved the given
   * Epics::ProcessVariable. If the given Epics::ProcessVariable is already
   * connected this function does nothing.
   * @warning The connection operations are buffer till the next
   * ChannelAccess::WaitForConnection call. It allows to buffer several request
   * that can be executed as an bulk request.
   *
   * @param rName PV name
   * @param rPV uninitialized Epics::ProcessVariable
   *
   * @throw Tools::Error_t in case the connection fails.
   */
  template <typename PV_t>
  static inline auto Connect(const std::string &rName, PV_t &rPV) {
    if (rPV.GetConnectionState() == cs_conn)
      return void();
    const auto status =
        ca_create_channel(rName.c_str(), nullptr, nullptr, 0, &rPV.m_ID);
    if (ECA_NORMAL != status)
      throw Tools::Error_t(Tools::Error_t::Type_t::Warning, m_gName,
                           "Failed to connect to " + rName +
                               ", got: " + std::string(ca_message(status)));
  }

  /**
   * @brief Disconnect disconnected the given Epics::ProcessVariable only the
   * channel is still connected. Otherwise this call does nothing.
   * @param rPV a connected Epics::ProcessVariable
   */
  template <typename PV_t> inline auto Disconnect(PV_t &rPV) const noexcept {
    const auto state =
        (nullptr != rPV.m_ID) ? ca_state(rPV.m_ID) : cs_never_conn;
    if ((state != cs_never_conn) && (state != cs_closed))
      ca_clear_channel(rPV.m_ID);
    rPV.m_ID = nullptr;
  }

  /**
   * @brief WaitForConnection block for the given timeout in seconds waiting for
   * PV connection.
   * @param timeout max timeout in seconds.
   * @return ECA_NORMAL if all connection requests have been performed.
   */
  static inline auto WaitForConnection(const ca_real timeout) noexcept {
    return ca_pend_io(timeout);
  }

  /*****************************************************************************
   * Pushing and pulling
   ****************************************************************************/
  /**
   * @brief BlockingPush upload the value stored in Epics::ProcessVariable to
   * the IOC. It blocks until the value has been uploaded or the timeout has
   * expired.
   * @param rPV Epics::ProcessVariable
   * @param timeout time out in seconds
   * @throw Tools::Error_t if the push times out or any error happens during the
   * transfer or preparation.
   */
  template <typename PV_t>
  static inline auto BlockingPush(const PV_t &rPV, const ca_real timeout = 1.) {
    using Tools::Error_t;

    auto status =
        ca_array_put(PV_t::m_gTypeIndex, rPV.GetSize(), rPV.m_ID, &rPV.m_Value);

    if (ECA_NORMAL != status)
      throw Error_t(Error_t::Type_t::Warning, m_gName,
                    "Failed to push to " + rPV.GetName() +
                        " , got: " + ca_message(status));

    status = ca_pend_io(timeout);
    if (ECA_NORMAL != status)
      throw Error_t(Error_t::Type_t::Warning, m_gName,
                    "Failed to push to " + rPV.GetName() +
                        " , got: " + ca_message(status));
  }

  /**
   * @brief Push buffers an upload request with an callback function that the
   * server has to acknowledge.
   * @details The request buffer is flushes once
   * ChannelAccess::FlushIO, ChannelAccess::WaitIOComplete has been called or
   * ChannelAccess::BlockingPush. All those functions flushes the internal I/O
   * buffer of the `Channel Access` libraries.
   *
   * @param rPV Epics::ProcessVariable
   * @return buffering status
   */
  template <typename PV_t> inline auto Push(PV_t &rPV) noexcept {
    const auto status = ca_array_put_callback(
        PV_t::m_gTypeIndex, rPV.GetSize(), rPV.m_ID, &rPV.m_Value,
        static_cast<caEventCallBackFunc *>(&ChannelAccess::OnPushComplete),
        this);

    if ((ECA_NORMAL == status) &&
        (0 == m_Pushes.fetch_add(1, std::memory_order_acq_rel)))
      m_PushStatus.store(ECA_NORMAL, std::memory_order_release);
    return status;
  }

  /**
   * @brief BlockingPull updates the internal storage of the given PV with the
   * value from server.
   * @param rPV Epics::ProcessVariable to be updated with the server value
   * @param timeout timeout in sec
   * @throw Tools::Error_t if timeout expires or any error during buffering ans
   * transmission.
   */
  template <typename PV_t>
  static inline auto BlockingPull(PV_t &rPV, const ca_real timeout = 3.) {
    using Tools::Error_t;

    auto status =
        ca_array_get(PV_t::m_gTypeIndex, rPV.GetSize(), rPV.m_ID, &rPV.m_Value);

    if (ECA_NORMAL != status)
      throw Error_t(Error_t::Type_t::Warning, m_gName,
                    "Failed to pull to " + rPV.GetName() +
                        " , got: " + ca_message(status));

    status = ca_pend_io(timeout);
    if (ECA_NORMAL != status)
      throw Error_t(Error_t::Type_t::Warning, m_gName,
                    "Failed to pull to " + rPV.GetName() +
                        " , got: " + ca_message(status));
  }

  /**
   * @brief Pull just registers the update request.
   * @param rPV Epics::ProcessVariable to be updated with the server value
   * @return status of buffering
   */
  template <typename PV_t> inline auto Pull(PV_t &rPV) const noexcept {
    return ca_array_get(PV_t::m_gTypeIndex, rPV.GetSize(), rPV.m_ID,
                        &rPV.m_Value);
  }

  /**
   * @brief FlushIO flushed the internal `Channel Access` I/O buffer.
   */
  static inline auto FlushIO() noexcept { return ca_flush_io(); }

  /**
   * @brief GetOutstandingPushes return number of request not yet acknowledged
   * by the server.
   */
  inline auto GetOutstandingPushes() const noexcept {
    return m_Pushes.load(std::memory_order_acquire);
  }

  /**
   * @brief GetLastPushStatus returns the status of the Server acknowledgment.
   */
  inline auto GetLastPushStatus() const noexcept {
    return m_PushStatus.load(std::memory_order_acquire);
  }

  /**
   * @brief WaitIOComplete waits the given timeout for out standing I/O to
   * complete.
   * @param timeout timeout in seconds.
   * @return ECA_NOMRAL if IO all outstanding IOCs have finished.
   */
  inline static auto WaitIOComplete(const ca_real timeout) noexcept {
    return ca_pend_io(timeout);
  }

private:
  /**
   * @brief OnPushComplete is the callback function executed once the server
   * acknowledges the Push request.
   * @details It decrements the internal counter saving the number of
   * outstanding confirmations.
   * @param args contains a user ptr providing callback to this class.
   */
  static auto OnPushComplete(event_handler_args args) noexcept -> void;

private:
  static constexpr auto m_gName =
      "ChannelAccess";                   /**< name shown in the logger */
  mutable std::atomic<int32_t> m_Pushes; /**< number of outstanding pushes */
  mutable std::atomic<int32_t>
      m_PushStatus; /**< last completed server ack. status */
};

} // namespace Epics
} // namespace SVD
