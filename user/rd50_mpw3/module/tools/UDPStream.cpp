#include "../tools/UDPStream.h"

namespace SVD {
namespace Tools {

UDPStream::UDPStream(const sockaddr_in &addr, int socket_handle) noexcept
    : m_Server(addr), m_Handle(socket_handle) {}

UDPStream::~UDPStream() {
  if (m_Handle >= 0) {
    close(m_Handle);
    m_Handle = UDPStream::m_gInvalidSocket;
  }
}

UDPStream &UDPStream::operator=(UDPStream &&rOther) noexcept {
  std::swap(m_Server, rOther.m_Server);
  std::swap(m_Handle, rOther.m_Handle);
  return *this;
}

UDPStream::UDPStream(UDPStream &&rOther) noexcept
    : m_Server(rOther.m_Server), m_Handle(rOther.m_Handle) {
  rOther.m_Handle = UDPStream::m_gInvalidSocket;
}

} // namespace Tools
} // namespace SVD
