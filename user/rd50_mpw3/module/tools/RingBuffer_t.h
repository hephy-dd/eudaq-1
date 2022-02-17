#ifndef RINGBUFFER_T_H
#define RINGBUFFER_T_H

#include <array>
#include <atomic>
#include <vector>

namespace SVD {
namespace Tools {

/**
 * @brief Single producer - single consumer ring buffer.
 *
 * @details Thread safety is ensured if only one producer pushing the data onto
 * the buffer and only one consumer popping the data. The buffer is synchronized
 * with 2 atomic indices, one representing a slot for reading and the other slot
 * for writing. The buffer is full implemented as:
 * ```cpp
 * isFull = [write index + 1] % [size of the buffer] == [read index]
 * ```
 *
 * The buffer is empty if the following is met:
 * ```cpp
 * [write index] == [read index].
 * ```
 *
 * This class only work with data structures that has a overloaded `void
 * swap(Data_t&, Data_t&) noexcept` function or works with std::swap(Data_t&,
 * Data_t&).
 *
 * In case of a full buffer the RingBuffer_t::Push(Data_t&), used to swap the
 * data with the one indicated by the write index inside the buffer,
 * return false and otherwise true. The same goes for the Pop(Data_t&) function,
 * that return false if the buffer is empty.
 *
 * @note: if the return value is false the given data is not modified.
 * @note since C++11 the std::swap is implemented in term of move operation.
 * Thus every data struct with the move ctor and move assignment enabled can be
 * used with this module.
 *
 */
template <typename Data_t, size_t SIZE> struct RingBuffer_t {
public:
  /**
   * @brief default ctor Resets the atomic counters to 0 and allocates the given
   * size for the buffer.
   */
  RingBuffer_t() noexcept : m_Read(0u), m_Write(0u), m_Buffer(SIZE) {
    static_assert(SIZE != 0, "Can not create RingBuffer_t with size = 0.");
  }

  ~RingBuffer_t() = default;

  RingBuffer_t(const RingBuffer_t &) = delete;
  RingBuffer_t(RingBuffer_t &&) = delete;

  inline auto IsFull() const noexcept {
    const auto curWrite = m_Write.load(std::memory_order_acquire);
    if (((curWrite + 1) % SIZE) == m_Read.load(std::memory_order_acquire))
      return true;
    return false;
  }

  inline auto IsEmpty() const noexcept {
    if (m_Read.load(std::memory_order_acquire) ==
        m_Write.load(std::memory_order_acquire))
      return true;
    return false;
  }

  /**
   * @brief Pushes the data onto the RingBuffer_t.
   * @details Returns true if the operation was successfully, otherwise false if
   * the buffer full. In case of an full buffer the given data is not modified!
   *
   * @param rData data to be swapped into the container
   * @return true if data had been push otherwise false.
   *
   * @warning if the pushing was successful the contained data is modified.
   */
  auto Push(Data_t &rData) noexcept {
    const auto curWrite = m_Write.load(std::memory_order_acquire);
    if (((curWrite + 1) % SIZE) == m_Read.load(std::memory_order_acquire))
      return false;

    using std::swap;
    swap(rData, m_Buffer[curWrite]);

    m_Write.store((curWrite + 1) % SIZE, std::memory_order_release);
    return true;
  }

  /**
   * @brief Pops the data from the buffer.
   * @details Return true if the operation was successful, otherwise false. If
   * the popping fails the data inside the container are not modified.
   *
   * @param rData swapped able object
   * @return true if popping was successful
   *
   * @warning if the popping succeeds the original object is swapped into the
   * buffer.
   */
  auto Pop(Data_t &rData) noexcept {
    const auto curRead = m_Read.load(std::memory_order_acquire);
    if (curRead == m_Write.load(std::memory_order_acquire))
      return false;

    using std::swap;
    swap(rData, m_Buffer[curRead]);

    m_Read.store((curRead + 1) % SIZE, std::memory_order_release);
    return true;
  }

private:
  std::atomic<unsigned int> m_Read;  /**< index of buffer to be popped */
  std::atomic<unsigned int> m_Write; /**< index of buffer to be pushed */
  std::vector<Data_t> m_Buffer;      /**< buffer containing the default
                                          constructed data */
};

} // namespace Tools
} // namespace SVD

#endif // RINGBUFFER_T_H
