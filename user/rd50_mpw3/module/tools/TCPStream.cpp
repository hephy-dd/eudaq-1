#include "TCPStream.h"

namespace SVD {
namespace Tools {

TCPStream::TCPStream(int socketHandle, struct sockaddr_in *addr) noexcept
    : m_SocketHandle(socketHandle),
      m_IP(64, ' ') {
  inet_ntop(PF_INET,
            reinterpret_cast<struct in_addr *>(&(addr->sin_addr.s_addr)),
            &(m_IP)[0], m_IP.size() - 1);
}

ssize_t TCPStream::Send(const char *const pData, const ssize_t size) noexcept {
  auto nrSent = ssize_t{0};
  while (nrSent < size) {
    const auto curSent = send(m_SocketHandle, pData + nrSent / sizeof(char),
                                 size - nrSent, MSG_NOSIGNAL);
    if (0 == curSent)
      return TransmissionState::CLOSED;
    else if (0 > curSent)
      return TransmissionState::ERROR;
    nrSent += curSent;
  }

  return nrSent;
}

bool TCPStream::IsReadyToRead(const unsigned int TIMEOUT) noexcept {
  fd_set socket_handels;
  FD_ZERO(&socket_handels);
  FD_SET(m_SocketHandle, &socket_handels);

  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  return (select(m_SocketHandle + 1, &socket_handels, nullptr, nullptr, &tv) >
          0)
             ? true
             : false;
}

TCPStream::~TCPStream() { close(m_SocketHandle); }

} // end namesapce
}
