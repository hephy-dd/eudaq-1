#include "../tools/EpicsLogger.h"
//#include "../tools/Error_t.h"

//#include "output_stream_stomp.h"

//using SuS::logfile::logger;
//using SuS::logfile::output_stream_stomp;

//namespace SVD {
//namespace Tools {

//EpicsLogger::EpicsLogger(const std::string &rName,
//                         const logger::log_level minLevel)
//    : m_Name(rName), m_System(0) {
//  auto *const pInstance = logger::instance();
//  try {
//    pInstance->set_subsystem_min_log_level(m_System, minLevel);
//  } catch (...) {
//    throw Error_t(Error_t::Type_t::Fatal, "EpicsLogger",
//                  "Failed to min log level");
//  }

//  auto pStream = std::unique_ptr<output_stream_stomp>(nullptr);
//  try {
//    m_System = pInstance->find_subsystem(m_Name);
//  } catch (const std::invalid_argument &) {
//    try {
//      pInstance->register_subsystem(m_Name);
//      pStream =
//          std::make_unique<output_stream_stomp>(m_Name, std::string(m_gSTOMP));
//      m_System = pInstance->find_subsystem(m_Name);
//      pInstance->add_output_stream(pStream.get(), m_Name);
//      pStream.release();
//    } catch (const std::bad_alloc &) {
//      throw Error_t(Error_t::Type_t::Warning, "EpicsLogger",
//                    "Failed to allocate logger!");
//    } catch (...) {
//      throw Error_t(Error_t::Type_t::Warning, "EpicsLogger",
//                    "Failed to add Logger!");
//    }
//  }
//}

//EpicsLogger::EpicsLogger(EpicsLogger &&rOther) noexcept
//    : m_Name(std::move(rOther.m_Name)), m_System(rOther.m_System) {}

//EpicsLogger &EpicsLogger::operator=(EpicsLogger &&rOther) noexcept {
//  m_Name = std::move(rOther.m_Name);
//  m_System = rOther.m_System;

//  return *this;
//}

//EpicsLogger::~EpicsLogger() {
//  auto failedRemove = true;
//  try {
//    auto *const pInstance = logger::instance();
//    failedRemove = !pInstance->remove_output_stream(m_Name);
//  } catch (...) {
//  }
//  if (failedRemove)
//    std::cerr << "Failed to remove Logger!!!" << std::endl;
//}

//void EpicsLogger::Log(const Logger::Type_t type, const std::string &,
//                      const std::string &rWho, const std::string &rWhat) const
//    noexcept {

//  auto epType = logger::log_level::finest;
//  switch (type) {
//  case Logger::Type_t::Info:
//    epType = logger::log_level::info;
//    break;
//  case Logger::Type_t::Warning:
//    epType = logger::log_level::warning;
//    break;
//  case Logger::Type_t::Unknown:
//  case Logger::Type_t::Fatal:
//    epType = logger::log_level::severe;
//    break;
//  }

//  std::stringstream ss;
//  ss << m_Name << "::" << rWho;
//  try {
//    logger::instance()->log(epType, m_System, rWhat, ss.str());
//  } catch (...) {
//    std::cerr << " failed to log to epics logger need"
//                 " and do not forget to add specific catch \n";
//  }
//}

//} // namespace Tools
//} // namespace SVD
