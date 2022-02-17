#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>

#include "../tools/Error_t.h"
#include "../tools/Logger.h"
#include "../tools/TCPAcceptor.h"
#include "../tools/TCPStream.h"

namespace SVD {
namespace Tools {

TCPAcceptor::TCPAcceptor(const std::string &rIP, const int port) noexcept
    : m_IP(rIP), m_SocketHandle(0), m_Port(port) {}

TCPAcceptor::~TCPAcceptor() {
  if (0 != m_SocketHandle)
    close(m_SocketHandle);
}

void TCPAcceptor::Start() {
  m_SocketHandle = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));

  address.sin_family = PF_INET;
  address.sin_port = htons(m_Port);

  if (!(m_IP.empty()))
    inet_pton(PF_INET, m_IP.c_str(), &(address.sin_addr));
  else
    address.sin_addr.s_addr = INADDR_ANY;

  int optval = 1;
  setsockopt(m_SocketHandle, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  int retval =
      bind(m_SocketHandle, reinterpret_cast<struct sockaddr *>(&address),
           sizeof(address));

  if (-1 == retval) {
    std::stringstream ss;
    ss << "Failed to bind listen-socket to port " << m_Port
       << ", got: " << strerror(errno);
    throw Error_t(Error_t::Type_t::Warning, m_gNAME, ss.str());
  }

  retval = listen(m_SocketHandle, 1);
  if (0 != retval) {
    std::stringstream ss;
    ss << "Listening to given port " << m_Port << ", got: " << strerror(errno);
    throw Error_t(Error_t::Type_t::Warning, m_gNAME, ss.str());
  }
}

std::unique_ptr<TCPStream> TCPAcceptor::Accept(const unsigned int timeout,
                                               const bool isBlocking) {
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  socklen_t size = sizeof(address);

  if (timeout) {
    // set socket non blocking
    int retval = fcntl(m_SocketHandle, F_GETFL, 0);
    if (retval < 0)
      throw Error_t(Error_t::Type_t::Warning, m_gNAME,
                    "Can not get listen socket flags.");

    fd_set set;
    FD_ZERO(&set);                /* clear the set */
    FD_SET(m_SocketHandle, &set); /* add file descriptor to the set */

    timeval time;
    time.tv_sec = timeout;
    time.tv_usec = 0;

    retval = select(1 + m_SocketHandle, &set, nullptr, nullptr, &time);
    if (0 >= retval)
      throw Error_t(Error_t::Type_t::Warning, m_gNAME, "Time out!");
  }

  int socket_handle = accept(
      m_SocketHandle, reinterpret_cast<struct sockaddr *>(&address), &size);

  if (-1 == socket_handle) {
    std::stringstream ss;
    ss << "Failed to accept connection from port: " << m_Port
       << ", got: " << strerror(errno);
    throw Error_t(Error_t::Type_t::Warning, m_gNAME, ss.str());
  }

  if (!isBlocking) {
    int retval = fcntl(socket_handle, F_GETFL, 0);
    if (retval < 0) {
      close(socket_handle);
      throw Error_t(Error_t::Type_t::Warning, m_gNAME,
                    "Can not read socket flags!");
    }

    if (0 != fcntl(socket_handle, F_SETFL, (retval | O_NONBLOCK))) {
      close(socket_handle);
      throw Error_t(Error_t::Type_t::Warning, m_gNAME,
                    "Can not set socket to non blocking!");
    }
  }

  return std::make_unique<TCPStream>(socket_handle, &address);
}

} // namespace Tools
} // namespace SVD
