#pragma once

#include <memory>

namespace SVD {
namespace Tools {

class TCPStream;

/**
 * @brief The TCPAcceptor class listens on the given IP and port for an
 * connection request. Upon receiving a connection request a different port is
 * bound to the request and a dedicated TCPStream is returned.
 */
class TCPAcceptor {
public:
  TCPAcceptor() = default;
  /**
   * @brief TCPAcceptor
   * @param rIP is the IP of the listening socket
   * @param port bound to the given IP.
   */
  TCPAcceptor(const std::string &rIP, const int port) noexcept;

  /**
   * closes the socket
   */
  ~TCPAcceptor();

  /**
   * @brief Start initializes this object after construction.
   */
  auto Start() -> void;

  /**
   * @brief Accept listens on the given IP:Port for connection requests.
   * @param timeout
   * @param isBlocking
   * @return std::unique_ptr<TCPStream>, if the connection has been established
   */
  auto Accept(const unsigned int timeout = 0, const bool isBlocking = true)
      -> std::unique_ptr<TCPStream>;

  /**
   * @brief GetPort returns the port.
   */
  inline auto GetPort() const noexcept { return m_Port; }

private:
  static constexpr auto m_gNAME = "TCPAcceptor";

  std::string m_IP; /**< IP/DNS address, per default the Acceptor is configured
                       with any address, ie this variable is usually empty. */
  int m_SocketHandle = 0; /**< socket handle */
  int m_Port = 0;         /**< listening port */
};

} // namespace Tools
} // namespace SVD
