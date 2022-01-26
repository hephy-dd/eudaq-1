#include "../tools/ChannelAccess.h"
#include <iostream>

namespace SVD {

using Tools::Error_t;
using Tools::Logger;

namespace Epics {

ChannelAccess::ChannelAccess(const bool preemptive)
    : m_Pushes(uint32_t{0}), m_PushStatus(ECA_NORMAL) {
  const auto status =
      ca_context_create(preemptive ? ca_enable_preemptive_callback
                                   : ca_disable_preemptive_callback);
  if (status != ECA_NORMAL)
    throw Error_t(Error_t::Type_t::Fatal, m_gName,
                  "Failed to create ca context, got " +
                      std::string(ca_message(status)));
}

ChannelAccess::~ChannelAccess() {
  for (auto iter = 0, out = this->GetOutstandingPushes();
       (iter < 30) && (0 != out); ++iter, out = this->GetOutstandingPushes()) {
    const auto status = ca_pend_io(1);
    if (status != ECA_NORMAL) {
      auto msg = std::stringstream();
      msg << out << " outstanding request still incomplete! Failed pend for "
          << "outstanding events, got: " << ca_message(status);

      Logger::Log(Logger::Type_t::Warning, m_gName, msg.str());
    }
  }
}

void ChannelAccess::OnPushComplete(event_handler_args args) noexcept {
  auto *const pAccess = static_cast<ChannelAccess *const>(args.usr);
  if (nullptr == pAccess)
    return void();

  if (ECA_NORMAL != args.status) {
    pAccess->m_PushStatus.store(args.status, std::memory_order_release);
    Logger::Log(Logger::Type_t::Warning, m_gName,
                std::string("OnPushComplete: ") + ca_message(args.status));
  }
  pAccess->m_Pushes.fetch_add(-1, std::memory_order_acq_rel);
}

} // namespace Epics
} // namespace SVD
