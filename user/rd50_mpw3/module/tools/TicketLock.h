#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace SVD {
namespace Tools {

template <typename Resource_t> class TicketGuard;
/**
 * @deprecated not used anymore
 */
template <typename Resource_t> class TicketLock {
public:
  TicketLock() noexcept : m_pResource(nullptr), m_Request(0u), m_Ticket(0u) {}
  TicketLock(Resource_t &&rResource) noexcept
      : m_pResource(std::move(rResource)), m_Request(0u), m_Ticket(0u) {}

  template <typename... Args_t>
  TicketLock(Args_t &&... rArgs) noexcept
      : m_pResource(std::forward<Args_t>(rArgs)...), m_Request(0u),
        m_Ticket(0u) {}

  // not copy able and move able
  TicketLock(TicketLock &&) = delete;
  TicketLock(const TicketLock &) = delete;

  ~TicketLock() {
    while (m_Request.load(std::memory_order_acquire) !=
           m_Ticket.load(std::memory_order_acquire))
      std::this_thread::sleep_for(m_gTimeOut);
  }

private:
  static constexpr auto m_gTimeOut = std::chrono::milliseconds(1);
  friend struct TicketGuard<Resource_t>;

  std::unique_ptr<Resource_t> m_pResource;
  std::atomic<uint32_t> m_Request;
  std::atomic<uint32_t> m_Ticket;
};

/**
 * @deprecated not used anymore.
 */
template <typename Resource_t> class TicketGuard {
public:
  TicketGuard(TicketLock<Resource_t> &rLock) noexcept
      : m_pResource(rLock.m_pResource.get()),
        m_Ticket(rLock.m_Ticket.fetch_add(1u, std::memory_order_release)),
        m_rCounter(rLock.m_Request) {
    while (m_Ticket != m_rCounter.load(std::memory_order_acquire))
      std::this_thread::sleep_for(TicketLock<Resource_t>::m_gTimeOut);
  }

  ~TicketGuard() { m_rCounter.fetch_add(1u, std::memory_order_release); }
  inline auto GetResource() const { return m_pResource; }

private:
  Resource_t *const m_pResource;
  const unsigned int m_Ticket;
  std::atomic<unsigned int> &m_rCounter;
};

} // namespace Tools
} // namespace SVD
