#pragma once

#include <cstring>
#include <memory>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>

namespace SVD {
namespace Tools {

class TCPStream;

/**
 * @brief The TCPConnector class proved a factory interface to connect to a TCP
 * server.
 * @see SVD::Tools::TCPStream
 */
class TCPConnector {
public:
  /**
   * @brief Connect connects to a TCP server. In case of an invalid connection a
   * std::unique_ptr<TCPStream> is returned!
   * @param port port
   * @param rIP IP or address from DNS
   * @return std::unique_ptr<TCPStream>, thus also the ownership.
   */
  auto Connect(const int port, const std::string &rIP)
      -> std::unique_ptr<TCPStream>;

private:
  static constexpr auto m_gName = "TCPConnector";
  /**
   * @brief ResolveHostName converts the given IP (string) to in_addr.
   * @param rHost IP as string
   * @param pAddr pointer to output in_addr.
   */
  auto ResolveHostName(const std::string &rHost, in_addr *const pAddr) const
      noexcept -> int;
};

} // namespace Tools
} // namespace SVD
