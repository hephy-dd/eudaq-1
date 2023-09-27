#include "XilinxEudaqInterface.h"
//#include "../tools/Logger.h"

#include <any>
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

      auto packId = data.back();
      data.pop_back();
      data.pop_back(); // UDP timestamp LSB, no one needs that anymore, legacy
                       // first sync approach, sucked heavily
      data.pop_back(); // UDP timestamp MSB
      data.push_back(packId); // forward package ID

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

Unpacker::Unpacker(PayloadBuffer_t &rBuffer, eudaq::LogSender *logger,
                   const std::string &name) noexcept
    : m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
      m_FADCBuffer(std::make_unique<FADCPayloadBuffer_t>()), m_euLogger(logger),
      m_name(name) {
  m_IsRunning->store(true);
  m_pThread = std::make_unique<std::thread>(&Unpacker::EventLoop, this,
                                            std::ref(rBuffer));
}

Unpacker::Unpacker(Unpacker &&rOther) noexcept
    : m_IsRunning(std::move(rOther.m_IsRunning)),
      m_FADCBuffer(std::move(rOther.m_FADCBuffer)),
      m_pThread(std::move(rOther.m_pThread)), m_name(rOther.m_name) {}

Unpacker::~Unpacker() {
  if (m_IsRunning != nullptr) {
    this->Exit();
  }
  if ((m_pThread.get() != nullptr) && m_pThread->joinable()) {
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
      frame.pop_back();
      // UDP package counter is located at last word of UDP packet, than
      // there is a 64 bit ovflw counter based on 50 ns added by the FPGA

      if (curPayload != ((++lastPayload) & 0xffffff)) {
        fadc.clear();
        auto ss = std::stringstream();
        ss << "Expected package number " << (lastPayload & 0xffffff)
           << ", got: " << curPayload << '!';
        std::cout << ss.str() << "\n";
        //        m_euLogger->SendLogMessage(
        //            eudaq::LogMessage(ss.str(), eudaq::LogMessage::LVL_WARN));
        lastPayload = curPayload;
      } else {
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

        while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
          //                std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        fadc.clear();
      }

      for (itBegin =
               std::find_if(itEnd, std::end(frame), &Unpacker::IsMainHeader);
           itBegin != std::end(frame);
           itBegin =
               std::find_if(itEnd, std::end(frame), &Unpacker::IsMainHeader)) {

        itEnd = std::find_if(itBegin, std::end(frame), &Unpacker::IsTrailer);
        //        std::cout << "got end\n";
        // const auto notSplit = itEnd != std::end(frame);
        const auto isTrailer = Unpacker::IsTrailer(*itEnd);
        fadc.resize(std::distance(itBegin, itEnd) + isTrailer);
        std::copy(itBegin, itEnd + isTrailer, fadc.begin());
        if (Unpacker::IsTrailer(fadc.back())) {
          while (!this->PushFADCFrame(fadc) && this->IsRunning()) {
            //                  std::this_thread::sleep_for(std::chrono::microseconds(10));
          }
          fadc.clear();
        } else {
        }
      }
    }

    if (fadc.size() > 5 * FADCBufferSize) {
      //      m_euLogger->SendLogMessage(
      //          eudaq::LogMessage("local buffer exceeds max size =>
      //          discarding",
      //                            eudaq::LogMessage::LVL_WARN));
      fadc.clear();
    }
  }
}

Sorter::Sorter(PayloadBuffer_t &rBuffer, eudaq::LogSender *logger) noexcept
    : m_PiggyBuffer(std::make_unique<FADCPayloadBuffer_t>()),
      m_BaseBuffer(std::make_unique<FADCPayloadBuffer_t>()),
      m_IsRunning(std::make_unique<std::atomic<bool>>(true)),
      m_euLogger(logger), m_pThread(std::make_unique<std::thread>(
                              &Sorter::eventLoop, this, std::ref(rBuffer))) {}

Sorter::~Sorter() {
  if ((m_pThread.get() != nullptr) && m_pThread->joinable()) {
    Exit();
    m_pThread->join();
  }
}

void Sorter::eventLoop(PayloadBuffer_t &rBuffer) noexcept {
  auto frame = Payload_t();
  auto piggyBuffer = FADCPayload_t();
  auto baseBuffer = FADCPayload_t();

  while (IsRunning()) {
    if (!NextFrame(rBuffer, frame)) {
      continue;
    }
    const auto curPayload =
        frame.back(); // we have to forward UDP packNmb to both Unpackers

    frame.pop_back();
    sortData(frame, piggyBuffer, true);
    if (piggyBuffer.size() > 0) {

      piggyBuffer.push_back(curPayload);
      ;
      while (!PushData(piggyBuffer, m_PiggyBuffer) && IsRunning()) {
      }
      piggyBuffer.clear();
    }
    sortData(frame, baseBuffer, false);
    if (baseBuffer.size() > 0) {
      baseBuffer.push_back(curPayload);
      while (!PushData(baseBuffer, m_BaseBuffer) && IsRunning()) {
      }
    }
  }
}

void Sorter::sortData(Payload_t &frame, FADCPayload_t &buffer,
                      bool piggy) noexcept {

  buffer.resize(m_gMaxBufferSize); // reserve space for worst case, full frame
                                   // contains only one type of data
  auto tester = Sorter::isPiggy;
  if (!piggy) {
    tester = Sorter::isBase;
  }
  if (std::count_if(frame.begin(), frame.end(), tester) == 0) {
    buffer.resize(0);
    return;
  }
  auto it = std::copy_if(frame.begin(), frame.end(), buffer.begin(),
                         tester); // it points to last copied element in buffer
  buffer.resize(
      std::distance(buffer.begin(), it)); // shrink buffer to needed size
}

bool Sorter::PushData(FADCPayload_t &newData,
                      std::unique_ptr<FADCPayloadBuffer_t> &buffer,
                      int retries) noexcept {

  for (; (--retries != 0) && !buffer->Push(newData);)
    ;
  if (0 == retries)
    return buffer->Push(newData);

  return true;
}

} // namespace UPDDetails

Merger::Merger(const BackEndID_t &rID,
               // std::shared_ptr<UPDDetails::PayloadBuffer_t> testBuffer,
               eudaq::LogSender &logger)
    : m_euLogger(&logger), m_Receiver(rID, &logger),
      m_Splitter(m_Receiver.GetBuffer(), &logger) {
  m_Event.m_Data.resize(1);
  m_Event.m_Words = 0;

  m_Unpackers.reserve(2);

  m_Unpackers.emplace_back(m_Splitter.GetBaseBuffer(), nullptr, "base");
  m_Unpackers.emplace_back(m_Splitter.GetPiggyBuffer(), nullptr, "piggy");
}

Merger::Merger(Merger &&rOther) noexcept
    : m_Receiver(std::move(rOther.m_Receiver)),
      m_Unpackers(std::move(rOther.m_Unpackers)),
      m_euLogger(rOther.m_euLogger) {}

Merger::~Merger() { this->Exit(); }

bool Merger::operator()(Event_t &rEvent) noexcept {

  if (std::all_of(std::begin(m_Unpackers), std::end(m_Unpackers),
                  [](const auto &rUnpacker) noexcept {
                    return rUnpacker.IsEmpty();
                  })) { // do nothing when unpackers have no data
    return false;
  }
  rEvent.m_Data.resize(m_Unpackers.size());
  rEvent.m_Words = 0;
  for (auto iFADC = 0ul; iFADC < m_Unpackers.size(); ++iFADC) {
    if (m_Unpackers[iFADC].IsEmpty()) {
      continue;
    }
    while (!m_Unpackers[iFADC].IsEmpty() &&
           !m_Unpackers[iFADC].PopFADCPayload(rEvent.m_Data[iFADC])) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    rEvent.m_Words += rEvent.m_Data[iFADC].size();
  }
  return true;
}

// namespace UPDDetails

} // namespace XLNX_CTRL
} // namespace SVD
