#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>

namespace SVD {
namespace Tools {

class UDPStream {
private:
  static constexpr auto m_gInvalidSocket = -1;

public:
  UDPStream() = default;
  UDPStream(UDPStream &&rOther) noexcept;
  UDPStream(const sockaddr_in &addr, int socket_handle) noexcept;
  UDPStream &operator=(UDPStream &&rOther) noexcept;
  UDPStream(const UDPStream &) = delete;
  UDPStream &operator=(const UDPStream &) = delete;
  ~UDPStream();

  inline auto Send(const char *const pData, size_t size) {
    const auto retval =
        sendto(m_Handle, pData, size, MSG_CONFIRM,
               reinterpret_cast<const sockaddr *>(&m_Server), sizeof(m_Server));

    if (retval == static_cast<ssize_t>(size)) {
      return void();
    } else if (retval == 0) {
      throw std::runtime_error("Socket closed");
    } else if (retval > 0) {
      throw std::runtime_error("Incomplete package");
    } else if (retval < 0) {
      throw std::runtime_error("Send failure, got: " +
                               std::string(strerror(errno)));
    }

    return void();
  }

  inline auto Receive(char *const pData, size_t size, int timeout = 0) const {
    if ((0 != timeout) && !this->IsReadyToRead(timeout))
      throw std::runtime_error("timeout");

    auto retval = recvfrom(m_Handle, pData, size, MSG_WAITALL, nullptr, 0);
    if (retval > 0) {
      return retval;
    } else if (retval == 0) {
      throw std::runtime_error("Socket closed");
    } else if (retval < 0) {
      throw std::runtime_error("Receive failure, got: " +
                               std::string(strerror(errno)));
    }

    return ssize_t{-1};
  }

  inline auto ReceiveUpto(char *const pData, size_t size,
                          int timeout = 0) const {
    if ((0 != timeout) && !this->IsReadyToRead(timeout))
      throw std::runtime_error("timeout");
    auto retval = recvfrom(m_Handle, pData, size, 0, nullptr, 0);
    if (retval > 0) {
      return retval;
    } else if (retval == 0) {
      throw std::runtime_error("Socket closed");
    } else if (retval < 0) {
      throw std::runtime_error("Receive failure, got: " +
                               std::string(strerror(errno)));
    }
    return ssize_t{-1};
  }

  inline auto IsReadyToRead(const unsigned int timeout) const noexcept -> bool {
    fd_set socket_handels;
    FD_ZERO(&socket_handels);
    FD_SET(m_Handle, &socket_handels);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    return (select(m_Handle + 1, &socket_handels, nullptr, nullptr, &tv) > 0)
               ? true
               : false;
  }

private:
  sockaddr_in m_Server;
  int m_Handle = -1;
};

} // namespace Tools
} // namespace SVD
