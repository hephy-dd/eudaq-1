#include "TCPConnector.h"
#include "TCPStream.h"

#include "../tools/Error_t.h"

namespace SVD {
namespace Tools {

std::unique_ptr<TCPStream> TCPConnector::Connect(const int port,
                                                 const std::string &rIP) {
  sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if (0 != this->ResolveHostName(rIP.data(), &address.sin_addr))
    inet_pton(PF_INET, rIP.c_str(), &address.sin_addr);

  auto handle = socket(AF_INET, SOCK_STREAM, 0);
  if (0 !=
      connect(handle, reinterpret_cast<sockaddr *>(&address), sizeof(address)))
    throw Error_t(Error_t::Type_t::Warning, m_gName,
                  "Unable to connect to server.");

  return std::make_unique<TCPStream>(handle, &address);
}

int TCPConnector::ResolveHostName(const std::string &rHost,
                                  in_addr *const pAddr) const noexcept {
  addrinfo *pInfo = nullptr;
  auto retval = getaddrinfo(rHost.c_str(), nullptr, nullptr, &pInfo);
  if (!retval)
    memcpy(pAddr, &(reinterpret_cast<sockaddr_in *>(pInfo->ai_addr))->sin_addr,
           sizeof(in_addr));
  if (nullptr != pInfo)
    freeaddrinfo(pInfo);
  return retval;
}

} // namespace Tools
} // namespace SVD
