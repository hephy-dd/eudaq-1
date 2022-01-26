#pragma once

#include "../tools/Logger.h"
// #include "logger.h"

//namespace SVD {
//namespace Tools {

//class EpicsLogger {
//public:
//  EpicsLogger(const std::string &rName,
//              const SuS::logfile::logger::log_level minLevel =
//                  SuS::logfile::logger::log_level::finest);

//  EpicsLogger(EpicsLogger &&rOther) noexcept;
//  EpicsLogger &operator=(EpicsLogger &&rOther) noexcept;

//  EpicsLogger(const EpicsLogger &) = delete;
//  EpicsLogger &operator=(const EpicsLogger &) = delete;

//  ~EpicsLogger();

//  auto Log(const Logger::Type_t type, const std::string &,
//           const std::string &rWho, const std::string &rWhat) const
//      noexcept -> void;
//  inline auto Flush() const noexcept -> void {}

//private:
//  static constexpr auto m_gSTOMP = "stomp://localhost:61613";

//  std::string m_Name;
//  SuS::logfile::logger::subsystem_t m_System;
//};

//} // end namespace SVD::Tools
//}
