#include "producer2guiproxy.h"

Producer2GUIProxy::Producer2GUIProxy(const std::string name,
                                     const std::string &runcontrol,
                                     QObject *parent)
    : QObject{parent}, eudaq::Producer(name, runcontrol) {}

void Producer2GUIProxy::DoInitialise() {
  auto ini = GetInitConfiguration();
  emit initialize(ini);
}

void Producer2GUIProxy::DoConfigure() {
  auto conf = GetConfiguration();
  emit configure(conf);
}

void Producer2GUIProxy::DoStartRun() { emit start(GetRunNumber()); }

void Producer2GUIProxy::DoStopRun() { emit stop(); }

void Producer2GUIProxy::DoReset() { emit reset(); }

void Producer2GUIProxy::DoTerminate() { emit terminate(); }
