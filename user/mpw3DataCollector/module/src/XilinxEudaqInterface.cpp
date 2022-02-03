#include "XilinxEudaqInterface.h"
#include "../tools/Logger.h"

#include <sstream>

namespace SVD {
using Tools::Logger;

namespace XLNX_CTRL {
  namespace UPDDetails {

    Receiver::Receiver(const BackEndID_t &rID)
        : m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
          m_Buffer(std::make_unique<PayloadBuffer_t>()),
          m_VMEBase(rID.m_Address) {
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
            auto ss = std::stringstream();
            ss << "Invalid data size received: '" << size
               << "' 32 bit words from FADC #" << m_VMEBase << '!';
            Logger::Log(Logger::Type_t::Warning, m_gName, ss.str());
          }
          if (data.empty())
            continue;

          auto iRet = 0;
          for (; (iRet < m_gRetries) && !m_Buffer->Push(data); ++iRet)
            ;
          if ((iRet == m_gRetries) && !m_Buffer->Push(data)) {
            auto ss = std::stringstream();
            ss << "Dropped payload packets, from FADC #"
               << VMEBaseFromPackage(data.back()) << '!';
            Logger::Log(Logger::Type_t::Warning, m_gName, ss.str());
          }
        } catch (const std::runtime_error &rError) {
          Logger::Log(Logger::Type_t::Warning, m_gName, rError.what());
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

    Unpacker::Unpacker(PayloadBuffer_t &rBuffer) noexcept
        : m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
          m_FADCBuffer(std::make_unique<FADCPayloadBuffer_t>()) {
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

    void Unpacker::EventLoop(PayloadBuffer_t &rBuffer) noexcept {
      auto frame = Payload_t();
      auto fadc = FADCPayload_t();
      auto lastPayload = 0;

      while (this->IsRunning()) {
        if (this->NextFrame(rBuffer, frame)) {
          const auto curPayload = PackageID(frame.back());
          if (curPayload != ((++lastPayload) & 0xffffff)) {
            fadc.clear();
            auto ss = std::stringstream();
            ss << "Expected package number " << (lastPayload & 0xffffff)
               << ", got: " << curPayload << '!';
            Logger::Log(Logger::Type_t::Warning, m_gName, ss.str());
            lastPayload = curPayload;
          }
        } else {
          continue;
        }

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
            while (!this->PushFADCFrame(fadc) && this->IsRunning())
              std::this_thread::sleep_for(std::chrono::microseconds(10));
            fadc.clear();
          }

          for (itBegin = std::find_if(itEnd, std::end(frame) - 1,
                                      &Unpacker::IsMainHeader);
               itBegin != std::end(frame) - 1;
               itBegin = std::find_if(itEnd, std::end(frame) - 1,
                                      &Unpacker::IsMainHeader)) {
            itEnd = std::find_if(itBegin, std::end(frame) - 1,
                                 &Unpacker::IsTrailer);
            // const auto notSplit = itEnd != std::end(frame);
            const auto isTrailer = Unpacker::IsTrailer(*itEnd);

            fadc.resize(std::distance(itBegin, itEnd) + isTrailer);
            std::copy(itBegin, itEnd + isTrailer, std::begin(fadc));
            if (Unpacker::IsTrailer(fadc.back())) {
              while (!this->PushFADCFrame(fadc) && this->IsRunning())
                std::this_thread::sleep_for(std::chrono::microseconds(10));
              fadc.clear();
            }
          }
        }
      }
    }

  } // namespace UPDDetails

  FADCGbEMerger::FADCGbEMerger(
      const std::vector<BackEndID_t> &rIDs,
      std::shared_ptr<UPDDetails::PayloadBuffer_t> testBuffer) {
    m_Event.m_Data.resize(rIDs.size());
    m_Event.m_Words = 0;
    m_Receivers.reserve(rIDs.size());
    m_Unpackers.reserve(rIDs.size());

    for (const auto &rID : rIDs) {
      //      m_Receivers.emplace_back(rID);
    }
    m_Unpackers.emplace_back(*testBuffer);
    //    for (auto &rReceiver : m_Receivers) {
    //                  m_Unpackers.emplace_back(rReceiver.GetBuffer());

    //    }
  }

  FADCGbEMerger::FADCGbEMerger(FADCGbEMerger &&rOther) noexcept
      : m_Receivers(std::move(rOther.m_Receivers)),
        m_Unpackers(std::move(rOther.m_Unpackers)) {}

  FADCGbEMerger::~FADCGbEMerger() { this->Exit(); }

  bool FADCGbEMerger::operator()(Event_t &rEvent) noexcept {
    if (std::any_of(
            std::begin(m_Unpackers), std::end(m_Unpackers),
            [](const auto &rUnpacker) noexcept { return rUnpacker.IsEmpty(); }))
      return false;

    rEvent.m_Data.resize(m_Unpackers.size());
    rEvent.m_Words = 0;
    for (auto iFADC = 0ul; iFADC < m_Unpackers.size(); ++iFADC) {
      while (!m_Unpackers[iFADC].PopFADCPayload(rEvent.m_Data[iFADC]))
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      rEvent.m_Words += rEvent.m_Data[iFADC].size();
    }

    const auto eventNr = (rEvent.m_Data.front().front() & 0x7fff);
    if (std::any_of(std::begin(rEvent.m_Data), std::end(rEvent.m_Data),
                    [eventNr](const auto &rData) noexcept {
                      return eventNr != (rData.front() & 0x7fff);
                    })) {
      Logger::Log(Logger::Type_t::Fatal, "FADCGbEMerger", "Event missmatch");
      auto expected = false;
      m_EventMissMatch.compare_exchange_strong(
          expected, true, std::memory_order_release, std::memory_order_relaxed);
      return false;
    }
    return true;
  }

  void FADCGbEMerger::SyncEvents() noexcept {
    if (!m_EventMissMatch.load(std::memory_order_acquire))
      return void();

    for (auto iFADC = 0ul; iFADC < m_Unpackers.size(); ++iFADC) {
      while (m_Unpackers[iFADC].PopFADCPayload(m_Event.m_Data[iFADC]))
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    m_EventMissMatch.store(false, std::memory_order_release);
  }

  // namespace UPDDetails

} // namespace XLNX_CTRL
} // namespace SVD
