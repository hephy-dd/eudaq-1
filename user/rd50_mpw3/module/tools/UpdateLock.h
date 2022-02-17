#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace SVD {
namespace Tools {

template <typename Res_t> struct WriteLock_t;
template <typename Res_t> struct ReadLock_t;

/**
 * @brief The UpdateResource_t struct provides an interface to 2 separate
 * resources (read and write ptr). The read ptr provides a storage to update
 * external views (ie GUI updates). The write ptr is used by the internal
 * processes to merge data.
 *
 * Once the internal thread updates the write ptr the interface try to swap the
 * r/w ptr during a read access. The purpose of this modules is trying to keep
 * the critical blocking section as small as possible.
 *
 * The external access to the each data container is controlled by the
 * ReadLock_t and WriteLock_t. Each of the access wrapper provides locking
 * duration mapped to their lifetime.
 */
template <typename Res_t> struct UpdateResource_t {
public:
  /**
   * @brief UpdateResource_t default ctor to allocate space on various STL
   * containers and wrappers.
   *
   * @warning The default constructed object is invalid!
   */
  UpdateResource_t() noexcept
      : m_MutexRead(), m_MutexWrite(), m_pRead(nullptr), m_pWrite(nullptr),
        m_IsUpdated(false) {}

  /**
   * @brief UpdateResource_t constructs the object with the given read and write
   * objects created outside the lock.
   *
   * @param rRead initialized read object
   * @param rWrite initialized write object
   */
  UpdateResource_t(std::unique_ptr<Res_t> &&rRead,
                   std::unique_ptr<Res_t> &&rWrite) noexcept
      : m_MutexRead(), m_MutexWrite(), m_pRead(std::move(rRead)),
        m_pWrite(std::move(rWrite)), m_IsUpdated(false) {}

  UpdateResource_t(const UpdateResource_t &) = delete;
  UpdateResource_t(UpdateResource_t &&) = delete;

  /**
   * @brief Reset resets the read and write ptrs in a blocking fashion.
   * @param rRead read ptr
   * @param rWrite write ptr
   */
  inline auto Reset(std::unique_ptr<Res_t> &&rRead,
                    std::unique_ptr<Res_t> &&rWrite) noexcept -> void {
    std::unique_lock<std::mutex> rlock(m_MutexRead, std::defer_lock);
    std::unique_lock<std::mutex> wlock(m_MutexWrite, std::defer_lock);

    std::lock(rlock, wlock);
    m_pRead = std::move(rRead);
    m_pWrite = std::move(rWrite);
  }

private:
  friend struct WriteLock_t<Res_t>; /**< allow WriteLock_t access to private
                                       members */
  friend struct ReadLock_t<Res_t>;  /**< allow ReadLock_t access to private
                                       members */

  std::mutex m_MutexRead;          /**< read mutex */
  std::mutex m_MutexWrite;         /**< write mutex */
  std::unique_ptr<Res_t> m_pRead;  /**< ptr to the read data */
  std::unique_ptr<Res_t> m_pWrite; /**< ptr to the write data */
  bool m_IsUpdated; /**< internal flag indicating an updated the write ptr and a
                       swap of r/w ptrs is required. */
};

/**
 * @brief The WriteLock_t struct implements an interface for the core system to
 * access and merge data produced by separate thread. There two main methods to
 * access data protected by the UpdateResource_t:
 *  - GetResource: non-blocking access and returns nullptr if the write lock is
 * already active.
 *  - GetResourceBlocking: blocks until the write lock can be acquired.
 *
 * @note Both methods sets the internal update flag to true once the lock has
 * been acquired. Thus also indicating to the ReadLock_t to try to swap the r/w
 * ptr on the next read ptr access.
 */
template <typename Res_t> struct WriteLock_t {
public:
  /**
   * @brief WriteLock_t access keeps a reference of the given UpdateResource_t.
   * During construction no mutexes are acquired.
   * @param rResource wrapper housing the update containers and mutexes.
   */
  WriteLock_t(UpdateResource_t<Res_t> &rResource) noexcept
      : m_rResource(rResource),
        m_WriteLock(rResource.m_MutexWrite, std::defer_lock) {}

  /**
   * @brief GetResource returns the const write ptr if the write lock has not
   * been acquired yet. In case of successfully accessing the lock a flag is set
   * to issue an swap during the next ReadLock_t access.
   * @return const write ptr if not locked, otherwise nullptr.
   */
  Res_t *GetResource() const noexcept {
    if ((m_WriteLock.owns_lock() || m_WriteLock.try_lock()) &&
        !m_rResource.m_IsUpdated) {
      m_rResource.m_IsUpdated = true;
      return m_rResource.m_pWrite.get();
    }

    return nullptr;
  }

  /**
   * @brief GetResource returns the const write ptr in a blocking fashion. After
   * acquiring a the write lock a flag is set to issue an swap during the next
   * ReadLock_t access.
   *
   * @warning This function might overwrite the last update depending on it's
   * access timing.
   *
   * @return const write ptr
   */
  Res_t *GetResourceBlocking() const noexcept {
    if (!m_WriteLock.owns_lock())
      m_WriteLock.lock();

    m_rResource.m_IsUpdated = true;
    return m_rResource.m_pWrite.get();
  }

private:
  UpdateResource_t<Res_t> &m_rResource; /**< reference to the resource data */
  mutable std::unique_lock<std::mutex>
      m_WriteLock; /**< unique lock to re-lock the mutex if needed */
};

/**
 * @brief The ReadLock_t struct implements the access for the consumer thread.
 * It also check if the write ptr has been updated, in that case it try to swap
 * the r/w ptrs before returning the data access.
 */
template <typename Res_t> struct ReadLock_t {
public:
  ReadLock_t(UpdateResource_t<Res_t> &rResource) noexcept
      : m_rResource(rResource),
        m_ReadLock(rResource.m_MutexRead, std::defer_lock),
        m_WriteLock(rResource.m_MutexWrite, std::defer_lock) {}

  /**
   * @brief GetResource check if another thread is accessing the write ptr and
   * tries to swap read and write ptrs! In case the write ptr has not been
   * updated it returns a nullptr.
   *
   * @return read ptr to a const Res_t or nullptr in case of:
   *  -# Read lock is still locked **or**
   *  -# Write ptr has not been updated!
   */
  const Res_t *GetResource() noexcept {
    if (m_ReadLock.owns_lock() || m_ReadLock.try_lock())
      return this->TrySwapRW() ? m_rResource.m_pRead.get() : nullptr;

    return nullptr;
  }

  /**
   * @brief GetResourceBlocking blocks until the read ptr becomes available.
   * during the block it tries to swap the write ptr.
   * @return read ptr to a const Res_t.
   */
  const Res_t *GetResourceBlocking() noexcept {
    if (!m_ReadLock.owns_lock())
      m_ReadLock.lock();
    this->TrySwapRW();
    return m_rResource.m_pRead.get();
  }

  /**
   * @brief GetReadResource only check if the read resource is locked. Tries to
   * swap r/w ptrs if the update flag is marked and returns the current read
   * ptr. In case of an successful swap the swap flag is reset.
   * @return read ptr to a const Res_t and nullptr in case the read ptr is
   * locked.
   */
  const Res_t *GetReadResource() noexcept {
    if (!m_ReadLock.owns_lock() && !m_ReadLock.try_lock())
      return nullptr;
    this->TrySwapRW();
    return m_rResource.m_pRead.get();
  }

private:
  /**
   * @brief TrySwapRW tries to swap read and write ptr if the write ptr has been
   * updated.
   * @return true if swapping was successful
   */
  inline auto TrySwapRW() noexcept -> bool {
    if (m_WriteLock.try_lock() && m_rResource.m_IsUpdated) {
      using std::swap;
      swap(m_rResource.m_pRead, m_rResource.m_pWrite);

      m_rResource.m_IsUpdated = false;
      m_WriteLock.unlock();
      return true;
    }
    return false;
  }

  UpdateResource_t<Res_t> &m_rResource;     /**< ref to the resource */
  std::unique_lock<std::mutex> m_ReadLock;  /**< read lock */
  std::unique_lock<std::mutex> m_WriteLock; /**< write lock */
};

} // namespace Tools
} // namespace SVD
