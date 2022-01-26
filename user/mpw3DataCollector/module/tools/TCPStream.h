#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <type_traits>

#include <cerrno>

namespace SVD {
namespace Tools {

/**
 * @brief The SocketProtocol_t enum is used to define some communication
 * protocols between FADC and QM.
 */
enum class SocketProtocol_t : uint32_t {
  AcceptConnection = 0xff000000, /**< In case the listening socket accepted the
                                    connection, issued by the FADC  */
  RejectConnection =
      0xff000001, /**< In case the listening socket has already
                     been bound to a client, issued by the FADC */
  RequestFinish =
      0xff000002, /**< Indicates the no data will be send any more! */
  AcknowledgeFinish = 0xff000003, /**< Indicates that RequestFinish has been
                                     read, issued by QM. */
};

/**
 * @brief The TCPStream class provides a RAII object for sockets as well as some
 * specialized r/w function to provide type safety.
 */
class TCPStream {
public:
  /**
   * @brief The TransmissionState enum indicated transmission status.
   */
  enum TransmissionState {
    INCOMPLETE = -128, /**< not all words has been sent correctly. */
    TIMEOUT = -3,      /**< timeout occurred */
    ERROR = -1,        /**< general error occurred */
    CLOSED = 0         /**< socket closed by either client or server */
  };

  /**
   * @brief TCPStream is the ctor called by
   * @param socketHandle socket handle created by either the TCPAcceptor or
   * TCPConnector.
   * @param addr struct address
   */
  TCPStream(int socketHandle, sockaddr_in *addr) noexcept;
  TCPStream(const TCPStream &) = delete;
  TCPStream(TCPStream &&other) = default;

  /**
   * closes the the socket handle.
   */
  ~TCPStream();

  /**
   * @brief Sends data stored in a STL container.
   * @note: the container has to be consecutive data layout (ie std::vector,
   * std::string, std::array). Data types that won't work: node based data and
   * c-string!
   * @param rData is a STL container with continues memory layout.
   * @return Container size in bytes, otherwise TransmissionState.
   * @see TransmissionState
   */
  template <typename Container_t>
  inline auto Send(const Container_t &rData) noexcept {
    const char *const pData = reinterpret_cast<const char *const>(rData.data());
    const size_t size = rData.size() * sizeof(typename Container_t::value_type);
    return this->Send(pData, size);
  }

  /**
   * @brief Sends a data type!
   * @param rData is a primitive data! Composite data types are allowed if it
   * does not contain heap storage! ie std::array<int, N> works while
   * std::vector<int> does not!
   * @return Data_t size in bytes, otherwise TransmissionState.
   * @see TransmissionState
   */
  template <typename Data_t>
  inline auto SendWord(const Data_t &rData) noexcept {
    const char *const pData = reinterpret_cast<const char *const>(&rData);
    const size_t size = sizeof(rData);
    return this->Send(pData, size);
  }

  /**
   * @brief Receive reads data depending on its internal size member!
   * @note Only works on STL container with continues memory layout.
   * @param rData is a STL container with continues memory layout.
   * @return Container size in bytes, otherwise TransmissionState.
   * @see TransmissionState
   */
  template <typename Container_t>
  inline auto Receive(Container_t &rData) noexcept -> ssize_t {
    const ssize_t size =
        rData.size() * sizeof(typename Container_t::value_type);
    auto *const pData = reinterpret_cast<char *const>(rData.data());
    ssize_t nrReceived = 0l;

    while (nrReceived < size) {
      const ssize_t curReceived = read(
          m_SocketHandle, pData + nrReceived / sizeof(char), size - nrReceived);
      if (0 == curReceived) {
        return TransmissionState::CLOSED;
      } else if (0 > curReceived) {
        return TransmissionState::ERROR;
      }
      nrReceived += curReceived;
    }

    return nrReceived;
  }

  /**
   * @brief ReceiveWord reads Data_t.
   * @param rData is a primitive data! Composite data types are allowed if it
   * does not contain heap storage! ie std::array<int, N> works while
   * std::vector<int> does not!
   * @return Data_t size in bytes, otherwise TransmissionState.
   * @see TransmissionState
   */
  template <typename Data_t>
  inline auto ReceiveWord(Data_t &rData) noexcept -> ssize_t {
    const ssize_t size = sizeof(Data_t);
    auto *const pData = reinterpret_cast<char *const>(&rData);
    ssize_t nrReceived = 0;

    while (nrReceived < size) {
      const ssize_t curReceived = read(
          m_SocketHandle, pData + nrReceived / sizeof(char), size - nrReceived);

      if (0 == curReceived)
        return TransmissionState::CLOSED;
      else if (0 > curReceived)
        return TransmissionState::ERROR;

      nrReceived += curReceived;
    }

    return nrReceived;
  }

  /**
   * @brief Receive overload to enable timeout
   * @param rData
   * @param timeout
   * @return Receive<Container_t>(Container_t &rData), or in case of a timeout
   * TransmissionState::TIMEOUT
   */
  template <typename Container_t>
  inline auto Receive(Container_t &rData, const unsigned int timeout) noexcept
      -> ssize_t {
    if (this->IsReadyToRead(timeout))
      return this->Receive(rData);
    return TransmissionState::TIMEOUT;
  }

  /**
   * @brief ReceiveWord overload to enable timeout
   * @param rData
   * @param timeout
   * @return ReceiveWord<Data_t>(Data_t &rData), or in case of a timeout
   * TransmissionState::TIMEOUT
   */
  template <typename Data_t>
  inline auto ReceiveWord(Data_t &rData, const unsigned int timeout) noexcept
      -> ssize_t {
    if (this->IsReadyToRead(timeout))
      return this->ReceiveWord(rData);
    return TransmissionState::TIMEOUT;
  }

private:
  friend class TCPConnector; /**< @deprecated had to move ctor to public for
                                std::make_unique */
  friend class TCPAcceptor;  /**< @deprecated had to move ctor to public for
                                std::make_unique */

  /**
   * @brief Send is in the internal abstraction to send the data over the
   * network.
   * @param pData a const pointer to const data
   * @param size of the data chunk
   * @return size sent, otherwise TransmissionState
   */
  ssize_t Send(const char *const pData, const ssize_t size) noexcept;

  /**
   * @brief IsReadyToRead checks if the socket buffer has entries.
   * @param TIMEOUT time out
   * @return true if ready to read, otherwise false (in case of a timeout)
   */
  bool IsReadyToRead(const unsigned int TIMEOUT) noexcept;

private:
  int m_SocketHandle; /**< socket handle */
  std::string m_IP;   /**< IP of the socket */
};

} // namespace Tools
} // namespace SVD
