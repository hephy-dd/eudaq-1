#include "XilinxEudaqInterface.h"
//#include "../tools/Logger.h"

#include <sstream>

namespace SVD {
//  using Tools::Logger;

namespace XLNX_CTRL {
namespace UPDDetails {

Receiver::Receiver(const BackEndID_t &rID, eudaq::LogSender *logger)
    : m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
      m_Buffer(std::make_unique<PayloadBuffer_t>()), m_VMEBase(rID.m_Address),
      m_euLogger(logger) {
  m_Socket = m_Acceptor(Receiver::IPFromID(m_VMEBase),
                        Receiver::PortFromID(m_VMEBase));
  m_pThread = std::make_unique<std::thread>(&Receiver::Eventloop, this);
}

Receiver::~Receiver() {
  if (m_pThread != nullptr && m_pThread->joinable()) {
    this->Exit();
    m_pThread->join();
  }
}

void Receiver::Eventloop() noexcept {
  auto data = Payload_t{};
  while (this->IsRunning()) {
    data.resize(m_gMaxPackageSize / sizeof(decltype(data)::value_type));
    try {
      auto size = m_Socket->ReceiveUpto(
          reinterpret_cast<char *>(data.data()),
          data.size() * sizeof(decltype(data)::value_type), m_gTimeout);

      if (size == -1)
        continue;

      size = size / sizeof(decltype(data)::value_type);

      if (static_cast<ssize_t>(data.size()) > size) {
        data.erase(data.begin() + size, data.end());
      } else {
        if (size != data.size()) {
          auto ss = std::stringstream();
          ss << "Invalid data size received: '" << size
             << "' 32 bit words from Xilinx #" << int(m_VMEBase);
          //              Logger::Log(Logger::Type_t::Warning, m_gName,
          //              ss.str());
          m_euLogger->SendLogMessage(
              eudaq::LogMessage(ss.str(), eudaq::LogMessage::LVL_WARN));
        }
      }
      if (data.empty())
        continue;

      using namespace std::chrono;
      auto now = duration_cast<microseconds>(
                     high_resolution_clock::now().time_since_epoch())
                     .count();
      uint32_t nowMsb = uint32_t(now >> 32);
      uint32_t nowLsb = now & 0xffffffff;
      data.push_back(nowMsb);
      data.push_back(nowLsb);

      auto iRet = 0;
      for (; (iRet < m_gRetries) && !m_Buffer->Push(data); ++iRet)
        ;
      if ((iRet == m_gRetries) && !m_Buffer->Push(data)) {
        auto ss = std::stringstream();
        ss << "Dropped payload packets, from FADC #"
           << VMEBaseFromPackage(data.back()) << '!';
        m_euLogger->SendLogMessage(
            eudaq::LogMessage(ss.str(), eudaq::LogMessage::LVL_WARN));
      }
    } catch (const std::runtime_error &rError) {
      m_euLogger->SendLogMessage(
          eudaq::LogMessage(std::string(m_gName) + ": " + rError.what(),
                            eudaq::LogMessage::LVL_WARN));
    }
  }

  m_Socket.reset(nullptr);
}

std::string Receiver::IPFromID(uint8_t vmeBase) noexcept {
  //  auto ss = std::stringstream();
  //  ss << static_cast<int>(m_gIP.front());
  //  for (auto idx = 1ul; idx < m_gIP.size(); ++idx)
  //    ss << '.' << static_cast<int>(m_gIP[idx]);
  //  ss << "192.168.201.";
  //  ss << static_cast<int>(vmeBase);
  //  return ss.str();
  return std::string();
}

Unpacker::Unpacker(PayloadBuffer_t &rBuffer, eudaq::LogSender *logger) noexcept
    : m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
      m_FADCBuffer(std::make_unique<FADCPayloadBuffer_t>()),
      m_euLogger(logger) {
  m_pThread = std::make_unique<std::thread>(&Unpacker::EventLoop, this,
                                            std::ref(rBuffer));
}

Unpacker::Unpacker(Unpacker &&rOther) noexcept
    : m_IsRunning(std::move(rOther.m_IsRunning)),
      m_FADCBuffer(std::move(rOther.m_FADCBuffer)),
      m_pThread(std::move(rOther.m_pThread)) {}

Unpacker::~Unpacker() {
  if ((m_pThread.get() != nullptr) && m_pThread->joinable()) {
    this->Exit();
    m_pThread->join();
  }
}

SVD::XLNX_CTRL::UPDDetails::SyncMode
SVD::XLNX_CTRL::UPDDetails::Unpacker::syncMode() const {
  return mSyncMode;
}

void SVD::XLNX_CTRL::UPDDetails::Unpacker::setSyncMode(SyncMode newSyncMode) {
  mSyncMode = newSyncMode;
}

void Unpacker::EventLoop(PayloadBuffer_t &rBuffer) noexcept {
  auto frame = Payload_t();
  auto fadc = FADCPayload_t();
  auto lastPayload = 0;
  uint32_t fw64BitTsMsb, fw64BitTsLsb, cpuTsLsb, cpuTsMsb;

  while (this->IsRunning()) {
    if (this->NextFrame(rBuffer, frame)) {
      cpuTsLsb = frame.back();
      frame.pop_back();
      cpuTsMsb = frame.back();
      frame.pop_back();
      //      uint64_t cpuTs64Bit =
      //          uint64_t(cpuTsLsb) + (uint64_t(cpuTsMsb) << uint64_t(32));
      const auto curPayload = PackageID(frame.back());
      frame.pop_back();
      // UDP package counter is located at last word of UDP packet, than
      // there is a 64 bit ovflw counter based on 50 ns added by the FPGA

      if (curPayload != ((++lastPayload) & 0xffffff)) {
        fadc.clear();
        auto ss = std::stringstream();
        ss << "Expected package number " << (lastPayload & 0xffffff)
           << ", got: " << curPayload << '!';
        m_euLogger->SendLogMessage(
            eudaq::LogMessage(ss.str(), eudaq::LogMessage::LVL_WARN));
        lastPayload = curPayload;
      } else {
      }
    } else {
      continue;
    }

    fw64BitTsMsb = frame.back();
    frame.pop_back();
    fw64BitTsLsb = frame.back();
    frame.pop_back();
    //    uint64_t fw64BitOvflwFull =
    //        (uint64_t(fw64BitTsMsb) << uint64_t(32)) + uint64_t(fw64BitTsLsb);
    if (mSyncMode == SyncMode::Timestamp) {
      auto itBegin = fadc.empty()
                         ? std::find_if(std::begin(frame), std::end(frame),
                                        &Unpacker::IsMainHeader)
                         : std::begin(frame);

      if (itBegin != std::end(frame)) {
        auto itEnd =
            std::find_if(itBegin, std::end(frame) - 1, &Unpacker::IsTrailer);

        const auto offset = fadc.size();
        const auto isTrailer = Unpacker::IsTrailer(*itEnd);
        fadc.resize(offset + std::distance(itBegin, itEnd + isTrailer));
        std::copy(itBegin, itEnd + isTrailer, fadc.begin() + offset);

        if (Unpacker::IsMainHeader(fadc.front()) &&
            Unpacker::IsTrailer(fadc.back())) {
          fadc.push_back(fw64BitTsMsb);

          fadc.push_back(fw64BitTsLsb);
          fadc.push_back(cpuTsMsb);
          fadc.push_back(cpuTsLsb);
          while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
            //                std::this_thread::sleep_for(std::chrono::microseconds(10));
          }
          fadc.clear();
        }

        for (itBegin = std::find_if(itEnd, std::end(frame) - 1,
                                    &Unpacker::IsMainHeader);
             itBegin != std::end(frame) - 1;
             itBegin = std::find_if(itEnd, std::end(frame) - 1,
                                    &Unpacker::IsMainHeader)) {

          itEnd =
              std::find_if(itBegin, std::end(frame) - 1, &Unpacker::IsTrailer);
          // const auto notSplit = itEnd != std::end(frame);
          const auto isTrailer = Unpacker::IsTrailer(*itEnd);

          fadc.resize(std::distance(itBegin, itEnd) + isTrailer);
          std::copy(itBegin, itEnd + isTrailer, std::begin(fadc));
          if (Unpacker::IsTrailer(fadc.back())) {
            fadc.push_back(fw64BitTsMsb);
            fadc.push_back(fw64BitTsLsb);
            fadc.push_back(cpuTsMsb);
            fadc.push_back(cpuTsLsb);
            while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
              //                  std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            fadc.clear();
          }
        }
      }
    } else if (mSyncMode == SyncMode::TriggerNumber) {
      /*
       * in this mode we don't split on SOF / EOF but on trigger words
       * these are indicated by a 16 bit header of 0xffff
       * We search within / over multiple UDP packages for
       * trigger word split events like (T ... Trigger word, D ... regular data
       * word)
       * <-- D D D D T D D D D D D D D D D T D D D D D D D D T D D T D D D D -->
       * Such a data stream would get split into the following pieces
       * <-----0----|----------1----------|--------2--------|--3--|-----4------>
       * Here 0, 1, 2, 3 would get shipped to the next stage as triggered
       * events, while 4 would still remain here until the next T comes by
       */

      // if we still have data buffered from the last frame
      auto itCurrTrg =
          fadc.empty() ? std::find_if(frame.begin(), frame.end(),
                                      &Unpacker::IsTriggerHeader)
                       // when no data buffered we look for 1st trigger word
                       // enclosed pack in current frame
                       : frame.begin(); // trigger pack was not closed before,
                                        // all data from very beginning belongs
                                        // to trigger pack from previous frame
      //      std::cout << "trigger found: " << *itCurrTrg << "\n";
      //      for (const auto &word : frame) {
      //        std::cout << std::hex << word << " ";
      //      }
      //      std::cout << "\n";

      if (itCurrTrg == frame.end()) {
        m_euLogger->SendLogMessage(eudaq::LogMessage(
            "not a single trigger word found in current frame, droping it",
            eudaq::LogMessage::LVL_WARN));
        continue;
      }
      auto itNextTrg =
          std::find_if(itCurrTrg + 1, frame.end(), &Unpacker::IsTriggerHeader);

      //      std::cout << "next trigger " << *itNextTrg << "\n";

      const auto offset = fadc.size();
      fadc.resize(offset + std::distance(itCurrTrg, itNextTrg));
      std::copy(itCurrTrg, itNextTrg, fadc.begin() + offset);

      if (Unpacker::IsTriggerHeader(fadc.front())) {
        fadc.push_back(fw64BitTsMsb);

        fadc.push_back(fw64BitTsLsb);
        fadc.push_back(cpuTsMsb);
        fadc.push_back(cpuTsLsb);

        while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
          // retry until success or somebody wants to stop us
        }
        fadc.clear();
      }
      // no more incomplete triggerWord enclosed packs left
      // search current frame for all remaining trigger packs
      for (itCurrTrg =
               std::find_if(itNextTrg, frame.end(), &Unpacker::IsTriggerHeader);
           itCurrTrg != frame.end();
           itCurrTrg = std::find_if(itNextTrg, frame.end(),
                                    &Unpacker::IsTriggerHeader)) {
        itNextTrg = std::find_if(itCurrTrg + 1, frame.end(),
                                 &Unpacker::IsTriggerHeader);

        fadc.resize(std::distance(itCurrTrg, itNextTrg));
        std::copy(itCurrTrg, itNextTrg, std::begin(fadc));

        if (Unpacker::IsTriggerHeader(*itNextTrg)) {
          // that's what we searched for so it must be a trigger word, just to
          // be save
          fadc.push_back(fw64BitTsMsb);
          fadc.push_back(fw64BitTsLsb);
          fadc.push_back(cpuTsMsb);
          fadc.push_back(cpuTsLsb);

          while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
            // retry until success or somebody wants to stop us
          }
          fadc.clear();
        } else {
        }
      }
    }
  }
}

} // namespace UPDDetails

FADCGbEMerger::FADCGbEMerger(
    const std::vector<BackEndID_t> &rIDs,
    // std::shared_ptr<UPDDetails::PayloadBuffer_t> testBuffer,
    eudaq::LogSender &logger)
    : m_euLogger(&logger) {
  m_Event.m_Data.resize(rIDs.size());
  m_Event.m_Words = 0;
  m_Receivers.reserve(rIDs.size());
  m_Unpackers.reserve(rIDs.size());

  for (const auto &rID : rIDs) {
    m_Receivers.emplace_back(rID, m_euLogger);
  }
  //      m_Unpackers.emplace_back(*testBuffer, logger);
  for (auto &rReceiver : m_Receivers) {
    m_Unpackers.emplace_back(rReceiver.GetBuffer(), m_euLogger);
  }
}

FADCGbEMerger::FADCGbEMerger(FADCGbEMerger &&rOther) noexcept
    : m_Receivers(std::move(rOther.m_Receivers)),
      m_Unpackers(std::move(rOther.m_Unpackers)),
      m_euLogger(rOther.m_euLogger) {}

FADCGbEMerger::~FADCGbEMerger() { this->Exit(); }

bool FADCGbEMerger::operator()(Event_t &rEvent) noexcept {
  if (std::any_of(std::begin(m_Unpackers), std::end(m_Unpackers),
                  [](const auto &rUnpacker) noexcept {
                    return rUnpacker.IsEmpty();
                  })) // do nothing when unpackers have no data
    return false;

  rEvent.m_Data.resize(m_Unpackers.size());
  rEvent.m_Words = 0;
  for (auto iFADC = 0ul; iFADC < m_Unpackers.size(); ++iFADC) {
    while (!m_Unpackers[iFADC].PopFADCPayload(rEvent.m_Data[iFADC]))
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    rEvent.m_Words += rEvent.m_Data[iFADC].size();
  }
  if (rEvent.m_Data.front().size() >= 4) {
    rEvent.m_recvTsCpu = rEvent.m_Data.front().back();
    rEvent.m_Data.front().pop_back();
    rEvent.m_recvTsCpu |= uint64_t(rEvent.m_Data.front().back()) << 32;
    rEvent.m_Data.front().pop_back();
    rEvent.m_recvTsFw = rEvent.m_Data.front().back();
    rEvent.m_Data.front().pop_back();
    rEvent.m_recvTsFw |= uint64_t(rEvent.m_Data.front().back()) << 32;
    rEvent.m_Data.front().pop_back();
    rEvent.m_EventNr = rEvent.m_Data.front().front() & 0xFFFF;
  } else {
    m_euLogger->SendLogMessage(
        eudaq::LogMessage("frame too small for TS extraction in merger: " +
                              std::to_string(rEvent.m_Data.front().size()),
                          eudaq::LogMessage::LVL_WARN));
    return false;
  }
  return true;
}

void SVD::XLNX_CTRL::FADCGbEMerger::setSyncMode(
    UPDDetails::SyncMode newSyncMode) {
  for (auto &unpacker : m_Unpackers) {
    unpacker.setSyncMode(newSyncMode);
  }
}

// namespace UPDDetails

} // namespace XLNX_CTRL
} // namespace SVD
