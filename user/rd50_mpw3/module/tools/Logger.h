#pragma once

#include "Error_t.h"
#include "TimeStamp.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include <algorithm>

namespace SVD {
namespace Tools {

class DefaultLogger;

/**
 * @brief The Logger class represents an interface to the logging system. This
 * class should not be used for debugging since it takes a mutex to synchronize
 * the output, thus might falsify some thread related error!
 * @note Logger implementation should be used with care since it's required to
 * lock... A MSG buffer is not implemented yet!
 */
class Logger {
public:
  /**
   * @brief The Type_t enum represents the MSG type.
   */
  enum class Type_t : unsigned char {
    Info,    /**< information */
    Warning, /**< warning */
    Fatal,   /**< alarm, error */
    Unknown, /**< given type can not be mapped to Type_t */
  };

  /**
   * @brief GetInstance return a lazy initialized logger
   * @return Logger instance
   */
  static inline auto GetInstance() noexcept -> Logger * {
    std::lock_guard<std::mutex> lock(m_gMutex);
    if (nullptr == m_Instance.get()) {
      m_Instance = std::unique_ptr<Logger>(new (std::nothrow) Logger());
    }
    return m_Instance.get();
  }

  /**
   * @brief Reset resets the logger.
   */
  static inline auto Reset() noexcept -> void {
    std::lock_guard<std::mutex> lock(Logger::m_gMutex);
    m_Instance.reset(nullptr);
  }

  /**
   * @brief Log pushed the MSG to all registered logger implementation.
   * @param type is the type of MSG.
   * @param rWho is object issuing the log.
   * @param rWhat is the actual MSG.
   */
  static inline void Log(const Type_t type, const std::string &rWho,
                         const std::string &rWhat) noexcept {
    auto *const pLogger = Logger::GetInstance();
    const auto time = pLogger->GetTime();
    std::lock_guard<std::mutex> lock(Logger::m_gMutex);
    for (auto &rLogger : pLogger->m_Loggers)
      rLogger->Log(type, time, rWho, rWhat);
  }

  /**
   * @brief Log converts a Error_t into an log MSG
   * @param rError
   */
  static inline void Log(const Error_t &rError) noexcept {
    auto type = Type_t::Unknown;
    switch (rError.GetType()) {
    case Error_t::Type_t::Fatal:
      type = Type_t::Fatal;
      break;
    case Error_t::Type_t::Warning:
      type = Type_t::Warning;
      break;
    }
    Logger::Log(type, rError.GetWho(), rError.GetWhat());
  }

  /**
   * @brief Flush calls the Flush function each Logger implementation.
   */
  static inline void Flush() noexcept {
    auto *const pLogger = Logger::GetInstance();
    for (auto &rLogger : pLogger->m_Loggers)
      rLogger->Flush();
  }

  /**
   * @brief AddLogger<Logger_t, ...> registers an logger by constructing it in
   * place. Logger_t is the type of the logger, while Args_t represents its ctor
   * arguments.
   */
  template <typename Logger_t, typename... Args_t>
  static inline void AddLogger(Args_t &&... rArgs) {
    auto *const pLogger = Logger::GetInstance();
    // lock each time expecting minor performance impact
    // used to prevent memory leak
    try {
      std::lock_guard<std::mutex> lock(Logger::m_gMutex);
      auto logger =
          std::make_unique<Model<Logger_t>>(std::forward<Args_t &&>(rArgs)...);
      // emplace back as concept ptr.
      pLogger->m_Loggers.emplace_back(static_cast<Concept *>(logger.release()));
    } catch (const std::bad_alloc &) {
      throw Error_t(Error_t::Type_t::Warning, "Logger", "Failed to add logger");
    } catch (const Error_t &rError) {
      throw rError;
    }
  }

  /**
   * @brief AddLogger<Logger_t> registers an logger by moving it into the list
   * of defined implementations.
   */
  template <typename Logger_t>
  static inline void AddLogger(Logger_t &&rLogger) noexcept {
    auto *const pLogger = Logger::GetInstance();

    std::lock_guard<std::mutex> lock(Logger::m_gMutex);
    auto logger = std::make_unique<Model<Logger_t>>(std::move(rLogger));
    pLogger->m_Loggers.emplace_back(static_cast<Concept *>(logger.get()));
    logger.release();
  }

private:
  static constexpr auto m_gLoggers =
      3; /**< number of expected logger, it just a estimated value to avoid
            reallocation. */

  /**
   * @brief Logger adds default logger to stdout.
   * @note if out of memory, system will just crash during construction.
   */
  Logger() noexcept;

  Logger(Logger &&) = delete;
  Logger(const Logger &) = delete;
  Logger &operator=(Logger &&) = delete;
  Logger &operator=(const Logger &) = delete;

  /**
   * @brief The Concept class represents the interface definition to the user
   * code.
   */
  class Concept {
  public:
    /**
     * @brief ~Concept enables inheritance.
     */
    virtual ~Concept();

    /**
     * @brief Log interface definition of the log function.
     * @param type type of the MSG
     * @param rTime time stamp
     * @param rWho object issuing the MSG
     * @param rWhat log MSG
     */
    virtual void Log(const Type_t type, const std::string &rTime,
                     const std::string &rWho,
                     const std::string &rWhat) noexcept = 0;

    /**
     * @brief Flush interface to flush the buffered MSGs.
     */
    virtual void Flush() noexcept = 0;
  };

  /**
   * @brief Model<Writer_t> is the interface definition to the implementation
   * code!
   */
  template <typename Writer_t> class Model : public Concept {
  public:
    Model() noexcept : Concept(), m_Writer() {}

    /**
     * @brief Model moves a already constructed implementation into the object.
     * @param rWriter already constructed and move-able implementation.
     */
    Model(Writer_t &&rWriter) noexcept
        : Concept(), m_Writer(std::move(rWriter)) {}

    /**
     * @brief Model constructs the implementation in place with the given ctor
     * arguments.
     * @param rArgs ctor arguments of the implementation.
     */
    template <typename... Args_t>
    Model(Args_t &&... rArgs) noexcept
        : Concept(), m_Writer(std::forward<Args_t>(rArgs)...) {}

    /**
     * @brief Final typed dtor preventing additional inheritance from the user
     * side.
     */
    virtual ~Model() final {}

    /**
     * @brief Log forward the information to the internally stored
     * implementation.
     * @param type MSG type
     * @param rTime time stamp
     * @param rWho object issuing the log
     * @param rWhat MSG to be logged
     */
    void Log(const Type_t type, const std::string &rTime,
             const std::string &rWho, const std::string &rWhat) noexcept final {
      m_Writer.Log(type, rTime, rWho, rWhat);
    }

    /**
     * @brief Flush forward to function call to the internally stored Impl_t.
     */
    void Flush() noexcept final { m_Writer.Flush(); }

  private:
    Writer_t m_Writer; /**< Logger implementation/adaption */
  };

  /**
   * @brief GetTime returns a time stamp.
   * @return time stamp
   */
  inline const std::string GetTime() const {
    return TimeStamp::Now("%Y-%m-%d %H:%M:%S %Z");
  }

private:
  std::vector<std::unique_ptr<Concept>>
      m_Loggers;              /**< container for registered loggers */
  static std::mutex m_gMutex; /**< mutex for thread safe logging */
  static std::unique_ptr<Logger> m_Instance; /**< Logger instance */
};

/**
 * @brief The DefaultLogger class is default stdout logger.
 */
class DefaultLogger {
public:
  /**
   * @brief Log streams the MSG to std::cout.
   * @param type MSG type
   * @param rTime time stamp
   * @param rWho object issuing the log
   * @param rWhat log MSG
   */
  void Log(const Logger::Type_t type, const std::string &rTime,
           const std::string &rWho, const std::string &rWhat) noexcept;

  /**
   * @brief Flush flushes stdout
   */
  void Flush() noexcept;
};

/**
 * @brief The DefaultFileLogger class implements a simple file logger, w/o file
 * rotation.
 */
class DefaultFileLogger {
public:
  /**
   * @brief DefaultFileLogger ctor taking an already created std::ostream
   * object.
   * @param pStream std::ostream objected wrapped by an unique_ptr to be moved
   * to the file logger
   */
  DefaultFileLogger(std::unique_ptr<std::ostream> &&pStream) noexcept;

  /**
   * @brief Log streams the information to the ostream.
   * @param type MSG type
   * @param rTime time stamp
   * @param rWho object issuing the log
   * @param rWhat log MSG
   */
  void Log(const Logger::Type_t type, const std::string &rTime,
           const std::string &rWho, const std::string &rWhat) noexcept;

  /**
   * @brief Flush flushes the std:::ostream
   */
  void Flush() noexcept;

private:
  std::unique_ptr<std::ostream> m_pStream; /**< internal std::ostream storage */
};

} // namespace Tools
} // namespace SVD
