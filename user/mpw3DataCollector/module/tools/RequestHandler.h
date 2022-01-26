#pragma once

#include "../tools/ChannelAccess.h"
#include "../tools/Logger.h"
#include "../tools/ProcessVariable.h"
#include "Request_t.h"

#include <chrono>
#include <memory>
#include <thread>

namespace SVD {
namespace Epics {

template <typename Worker_t> class RequestHandler {
public:
  RequestHandler() noexcept : m_pWorker(nullptr) {}

  RequestHandler(std::unique_ptr<Worker_t> &&rWorker) noexcept
      : m_pWorker(std::move(rWorker)) {}

  template <typename... Args_t>
  RequestHandler(Args_t &&... rArgs) noexcept
      : m_pWorker(std::make_unique<Worker_t>(std::forward<Args_t>(rArgs)...)) {}

  RequestHandler(RequestHandler<Worker_t> &&rOther) noexcept
      : m_pWorker(std::move(rOther.m_pWorker)) {}

  ~RequestHandler() { m_pWorker.reset(); }

  template <typename PV_t>
  inline auto operator()(const ChannelAccess &, PV_t &rRequest) noexcept {
    m_pWorker->operator()(rRequest.Get());
  }

  inline auto GetWorker() const noexcept { return m_pWorker.get(); }

  inline auto IsExitRequested() const noexcept {
    return m_pWorker->IsExitRequested();
  }

private:
  static constexpr auto m_gName = "RequestHandler";
  std::unique_ptr<Worker_t> m_pWorker;
};

} // namespace Epics
} // namespace SVD
