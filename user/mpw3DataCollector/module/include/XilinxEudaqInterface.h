#pragma once

#include "../tools/Defs.h"
#include "../tools/RingBuffer_t.h"
#include "../tools/UDPAcceptor.h"
#include "Event_t.h"

#include <algorithm>
#include <memory>
#include <string>

namespace SVD {
  namespace XLNX_CTRL {

    namespace UPDDetails {

      static constexpr auto PayloadBufferSize = 1024;
      using Payload_t = std::vector<Defs::VMEData_t>;
      using PayloadBuffer_t = Tools::RingBuffer_t<Payload_t, PayloadBufferSize>;

      static constexpr auto FADCBufferSize = 1024;
      using FADCPayload_t = std::vector<Defs::VMEData_t>;
      using FADCPayloadBuffer_t =
          Tools::RingBuffer_t<FADCPayload_t, FADCBufferSize>;

      inline constexpr auto
      VMEBaseFromPackage(const Defs::VMEData_t &rWord) noexcept {
        return rWord & 0xff;
      }

      inline constexpr auto PackageID(const Defs::VMEData_t &rWord) noexcept {
        return (rWord >> 8) & 0xffffff;
      }

      class Receiver {
      private:
        static constexpr auto m_gRetries = 100;
        static constexpr auto m_gMaxPackageSize = 8192;
        static constexpr auto m_gName = "UDPReceiver";
        static constexpr auto m_gTimeout = 250000;

        //  static constexpr auto m_gIP =
        //      std::array<uint8_t, 3>({uint8_t(192), uint8_t(168),
        //      uint8_t(201)});

        inline static auto constexpr PortFromID(uint8_t vmeBase) noexcept
            -> Defs::VMEData_t {
          return vmeBase | (0xc3 << 8);
        }

      public:
        Receiver() = default;
        Receiver(const BackEndID_t &rID);
        ~Receiver();
        Receiver(Receiver &&rOther) noexcept
            : m_IsRunning(std::move(rOther.m_IsRunning)),
              m_Socket(std::move(rOther.m_Socket)),
              m_Buffer(std::move(rOther.m_Buffer)),
              m_pThread(std::move(rOther.m_pThread)), m_Acceptor(),
              m_VMEBase(rOther.m_VMEBase) {}

        inline auto GetBuffer() noexcept -> PayloadBuffer_t & {
          return *m_Buffer;
        }

        inline auto IsRunning() const noexcept {
          return m_IsRunning->load(std::memory_order_acquire);
        }

        inline auto Exit() noexcept {
          m_IsRunning->store(false, std::memory_order_release);
        }

      private:
        static auto IPFromID(uint8_t vmeBase) noexcept -> std::string;
        auto Eventloop() noexcept -> void;

        std::unique_ptr<std::atomic<bool>> m_IsRunning{};
        std::unique_ptr<Tools::UDPStream> m_Socket{};
        std::unique_ptr<PayloadBuffer_t> m_Buffer{};
        std::unique_ptr<std::thread> m_pThread{};

        Tools::UDPAcceptor m_Acceptor{};

        uint8_t m_VMEBase = 0;
      };

      class Unpacker {
      public:
        Unpacker() = default;
        Unpacker(PayloadBuffer_t &rBuffer) noexcept;
        Unpacker(Unpacker &&rOther) noexcept;
        ~Unpacker();

        inline auto IsRunning() const noexcept {
          return m_IsRunning->load(std::memory_order_acquire);
        }

        auto Exit() noexcept {
          m_IsRunning->store(false, std::memory_order_release);
        }

        inline auto PopFADCPayload(FADCPayload_t &rPayload,
                                   int retries = 100) noexcept -> bool {
          while ((--retries != 0) && !m_FADCBuffer->Pop(rPayload))
            ;
          if (retries == 0)
            return m_FADCBuffer->Pop(rPayload);
          return true;
        }

        inline auto IsEmpty() const noexcept { return m_FADCBuffer->IsEmpty(); }

      private:
        static constexpr auto m_gName = "UDPUnpacker";
        inline auto NextFrame(PayloadBuffer_t &rBuffer, Payload_t &rFrame,
                              int retries = 100) noexcept -> bool {
          for (; (--retries != 0) && !rBuffer.Pop(rFrame);)
            ;
          if (0 == retries)
            return rBuffer.Pop(rFrame);
          return true;
        }

        static inline auto IsMainHeader(const Defs::VMEData_t &rWord) noexcept {
          return ((rWord >> 28) & 0xe) == 0xc;
        }

        static inline auto IsTrailer(const Defs::VMEData_t &rWord) noexcept {
          return 0xe == ((rWord >> 28) & 0xf);
        }

        inline auto PushFADCFrame(FADCPayload_t &rFADC,
                                  int retries = 100) noexcept {
          for (; (--retries != 0) && !m_FADCBuffer->Push(rFADC);)
            ;
          if (0 == retries)
            return m_FADCBuffer->Push(rFADC);
          return true;
        }

        auto EventLoop(PayloadBuffer_t &rBuffer) noexcept -> void;

      private:
        std::unique_ptr<std::atomic<bool>> m_IsRunning{};
        std::unique_ptr<FADCPayloadBuffer_t> m_FADCBuffer{};
        std::unique_ptr<std::thread> m_pThread{};
      };

    } // namespace UPDDetails

    class FADCGbEMerger {
    public:
      FADCGbEMerger(const std::vector<BackEndID_t> &rIDs,
                    std::shared_ptr<SVD::XLNX_CTRL::UPDDetails::PayloadBuffer_t>
                        testBuffer);
      FADCGbEMerger(const FADCGbEMerger &rOther) = delete;
      FADCGbEMerger(FADCGbEMerger &&rOther) noexcept;
      ~FADCGbEMerger();

      auto operator()(Event_t &rEvent) noexcept -> bool;

      inline auto IsEmpty() const noexcept -> bool {
        return std::all_of(
            std::begin(m_Unpackers), std::end(m_Unpackers),
            [](const auto &rUnpacker) noexcept { return rUnpacker.IsEmpty(); });
      }

      auto SyncEvents() noexcept -> void;

      inline auto IsEventMissmatch() const noexcept {
        return m_EventMissMatch.load(std::memory_order_acquire);
      }

    private:
      inline auto Exit() noexcept -> void {
        for (auto &rReceiver : m_Receivers)
          rReceiver.Exit();
        for (auto &rUnpacker : m_Unpackers)
          rUnpacker.Exit();
      }

      static inline auto
      GetEventNumber(const std::vector<Defs::VMEData_t> &rData) -> uint32_t {
        return (rData.front() & 0x7fff);
      }

      std::vector<UPDDetails::Receiver> m_Receivers{};
      std::vector<UPDDetails::Unpacker> m_Unpackers{};
      std::atomic<bool> m_EventMissMatch{false};
      Event_t m_Event;
    };

  } // namespace XLNX_CTRL
} // namespace SVD
