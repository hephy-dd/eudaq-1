#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../tools/Error_t.h"
#include "../tools/UDPStream.h"

namespace SVD {
namespace Tools {

namespace details {
static auto resolveHostName(const std::string &rHost, in_addr *const pAddr)
    -> int {
  addrinfo *pInfo = nullptr;
  auto retval = getaddrinfo(rHost.c_str(), nullptr, nullptr, &pInfo);
  if (!retval)
    memcpy(pAddr, &(reinterpret_cast<sockaddr_in *>(pInfo->ai_addr))->sin_addr,
           sizeof(in_addr));

  if (nullptr != pInfo)
    freeaddrinfo(pInfo);

  return retval;
}
} // namespace details

class UDPAcceptor {
public:
  inline auto operator()(const std::string &rIP, const uint16_t port) {
    auto address = sockaddr_in{};
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (rIP.empty()) {
      address.sin_addr.s_addr = INADDR_ANY;
    } else {
      if (0 != details::resolveHostName(rIP, &address.sin_addr))
        inet_pton(PF_INET, rIP.c_str(), &address.sin_addr);
    }

    auto handle = socket(AF_INET, SOCK_DGRAM, 0);
    if (handle < 0)
      throw Tools::Error_t(Tools::Error_t::Type_t::Fatal, "UDPAcceptor",
                           "Failed to create UDP stream, got: " +
                               std::string(strerror(errno)));

    auto bufferSize = 8192000;
    if (setsockopt(handle, SOL_SOCKET, SO_RCVBUF, &bufferSize,
                   sizeof(bufferSize)) == -1)
      throw Tools::Error_t(Tools::Error_t::Type_t::Fatal, "UDPAcceptor",
                           "Failed to set udp socket read buffersize, got: " +
                               std::string(strerror(errno)));

    const auto retval = bind(
        handle, reinterpret_cast<const sockaddr *>(&address), sizeof(address));
    if (retval < 0)
      throw Tools::Error_t(Tools::Error_t::Type_t::Fatal, "UDPAcceptor",
                           "Failed to bind UDP stream, got: " +
                               std::string(strerror(errno)));

    return std::make_unique<UDPStream>(address, handle);
  }
};

} // namespace Tools
} // namespace SVD
