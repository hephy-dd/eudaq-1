#pragma once

#include "../tools/ChannelAccess.h"
#include "../tools/Defs.h"
#include "../tools/ProcessVariable.h"

namespace SVD {
namespace Tools {

/**
 * @brief The PSChannel class implements the communication between IVScan,
 * VSepScan and SVD:PS.
 *
 * @details The communication consists of 3 PVs:
 *  - SVD:PS:VSet contains the voltage requested by the corresponding run.
 *  - SVD:PS:VConf houses the voltage applied by the SVD:PS and is used to
 * cross-check during IVScan and VSepScan if the configured voltage was the
 * requested one.
 *  - SVD:PS:[Prefix IV or VSep]:VRequest is the request PV starting the voltage
 * adjustment and also indicating if the request had been processed.
 *
 * The typical control flow goes as follows:
 *  -# the run type set VSet to a certain value and in turn pushes the VRequest
 * to VRequest_t::SetVoltage.
 *  -# The SVD:PS registers the change and reads the VSet to configure the
 * voltage.
 *  -# Once the voltage had been reached SVD:PS pushes the configured voltage
 * VConf and in turn resets the VRequest PV to VRequest::Processed indicating
 * the configuration has finished.
 *  -# the run type in turn compares the VConf with an internally stored value,
 * independent from the broadcasted value.
 *
 * The double checking described above is used to prevent synchronization
 * issues.
 *
 * @see VRequest_t
 */
class PSChannel {
public:
  using ChannelAccess = Epics::ChannelAccess;
  using Error_t = Tools::Error_t;
  using Logger = Tools::Logger;

  // PV definition
  template <int TypeIndex>
  using PV_t = Epics::ProcessVariable<TypeIndex, Epics::PVStruct_t::Value, 1>;

  /**
   * @brief The VRequest enum class used to perform handshake with PS.
   */
  enum class VRequest_t {
    Processed = 0, /**<  request has been processed by SVD:PS and SVD:PS:VConf
                      had been updated with the configured value. */
    SetVoltage =
        1,     /**< request SVD:PS to set the voltage, saved in SVD:PS:VSet */
    Finish = 2 /**< run finished and trigger a "stop run" request in PS state */
  };

  static constexpr auto m_gName = "PSChannel"; /**< name shown in the logger */
  static constexpr auto m_gRequestName = "Request"; /**< request PV name */
  static constexpr auto m_gVSetName =
      "SVD:PS:VSet"; /**< name of requested voltage */
  static constexpr auto m_gVConfName =
      "SVD:PS:VConf"; /**< name of configured voltage */
  static constexpr auto m_gStateName =
      "SVD:PS:State"; /**< PV name of SVD:PS state */

public:
  /**
   * @brief PSChannel Abstraction for n or p-side channel control.
   */
  PSChannel(ChannelAccess *const pAccess = nullptr) noexcept;

  /**
   * @brief dtor, disconnects all connected PVs if constructed with a pAccess !=
   * nullptr!
   */
  ~PSChannel();

  /**
   * @brief Connect connects the internal PVs.
   * @param rPrefix Prefix of the system
   * @throw SVD::Tools::Error_t, type Fatal! if PVs does not exist.
   */
  auto Connect(const std::string &rPrefix, const ca_real timeout) -> void;

  /**
   * @brief SetVoltage write the desired voltage to m_SetVoltage and in turn
   * m_WriteVoltage = 1;
   * @param voltage desired voltages
   * @throw SVD::Tools::Error_t. Warning if pushing values failed!
   */
  auto SetVoltage(const Defs::Float_t voltage) -> void;

  /**
   * @brief ConfirmVoltage check if the channel is ready and confirms the set
   * configured voltage with the requested one.
   * @param voltage read back voltage
   * @return true if channel is ready and if the set voltage differs from the
   * requested one by more then 0.1V.
   */
  auto ConfirmVoltage(const Defs::Float_t voltage) -> bool;

  /**
   * @brief SetFinished indicates if the run has finished
   */
  auto SetFinished() -> void;

private:
  /**
   * @brief IsChannelReady determines if the channel is ready, ie if the request
   * PV had been reset to VRequest::Processed.
   * @return weather the channel is ready or not
   * @throw Tools::Error_t. Warning if pulling values fails. Fatal if
   * channel state is disabled or m_IsOk is false or if the channel state
   * contains an invalid value.
   */
  auto IsChannelReady() -> bool;

  /**
   * @brief IsRequestedVoltage check a different PV that is adjusted after
   * handshake
   * @param voltage expected voltage
   * @return true the expected voltage and VConf differs by 0.001;
   * @throw Tools::Error_t if VConf PV has been disconnected or if the
   * BlockingPull of VConf PV fails or times out.
   */
  auto IsRequestedVoltage(const Defs::Float_t voltage) -> bool;

private:
  PV_t<DBR_ENUM> m_State;   /**< PS state PV */
  PV_t<DBR_ENUM> m_Request; /**< PV providing handshake */
  PV_t<DBR_FLOAT>
      m_VSet; /**< desired voltage set by FADC before set voltage request */
  PV_t<DBR_FLOAT> m_VConf; /**< configured voltage set by PS after request has
   been processed */

  ChannelAccess *const m_pAccess; /**< communication channel */
};

} // namespace Tools
} // namespace SVD
