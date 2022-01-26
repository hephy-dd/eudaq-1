#include "Logger.h"
// #include "EpicsLogger.h"

namespace SVD {
namespace Tools {

std::unique_ptr<Logger> Logger::m_Instance = std::unique_ptr<Logger>(nullptr);
std::mutex Logger::m_gMutex;

Logger::Logger() noexcept : m_Loggers() {
  m_Loggers.reserve(m_gLoggers);
  {
    auto tmp = std::make_unique<Logger::Model<DefaultLogger>>();
    this->m_Loggers.emplace_back(static_cast<Concept *>(tmp.get()));
    tmp.release();
  }
}

Logger::Concept::~Concept() {}

void DefaultLogger::Log(const Logger::Type_t type, const std::string &rTime,
                        const std::string &rWho,
                        const std::string &rWhat) noexcept {
  auto logType = std::string();
  switch (type) {
  case Logger::Type_t::Info:
    logType = "Info";
    break;
  case Logger::Type_t::Warning:
    logType = "Warning";
    break;
  case Logger::Type_t::Fatal:
    logType = "Error";
    break;
  case Logger::Type_t::Unknown:
    // default:
    logType = "Unknown Type";
    break;
  }

  if ((type == Logger::Type_t::Info) || (type == Logger::Type_t::Warning)) {
    std::cout << '[' << rTime << "]\t" << logType << "::" << rWho
              << "::" << rWhat << '\n';
  } else {
    std::cout << '[' << rTime << "]\t" << logType << "::" << rWho
              << "::" << rWhat << std::endl;
  }
}

void DefaultLogger::Flush() noexcept { std::cout.flush(); }

DefaultFileLogger::DefaultFileLogger(
    std::unique_ptr<std::ostream> &&pStream) noexcept
    : m_pStream(std::move(pStream)) {}

void DefaultFileLogger::Log(const Logger::Type_t type, const std::string &rTime,
                            const std::string &rWho,
                            const std::string &rWhat) noexcept {
  auto logType = std::string();
  switch (type) {
  case Logger::Type_t::Info:
    logType = "Info";
    break;
  case Logger::Type_t::Warning:
    logType = "Warning";
    break;
  case Logger::Type_t::Fatal:
    logType = "Error";
    break;
  case Logger::Type_t::Unknown:
  default:
    logType = "Unknown Type";
    break;
  }

  *m_pStream << '[' << rTime << "]\t" << logType << "::" << rWho
             << "::" << rWhat << '\n';

  if (type != Logger::Type_t::Info)
    this->Flush();
}

void DefaultFileLogger::Flush() noexcept { m_pStream->flush(); }

} // namespace Tools
} // namespace SVD
