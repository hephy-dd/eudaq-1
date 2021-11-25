#ifndef MPW3_MONITOR_H
#define MPW3_MONITOR_H

#include "eudaq/Monitor.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include "mpw3_qt_if.h"
#include <QObject>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <ratio>
#include <thread>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class Mpw3Monitor : public eudaq::Monitor {
public:
  Mpw3Monitor(const std::string &name, const std::string &runcontrol);
  ~Mpw3Monitor();
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoReceive(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("MPW3Monitor");

  Mpw3QtIf *getMpw3QtIf();

private:
  bool mEnPrint;
  bool mEnStdConverter;
  bool mEnStdPrint;
  bool mForwardToGui;

  Mpw3QtIf *mMpw3QtIf;
};
#endif // MPW3_MONITOR_H
