#include "mpw3_monitor.h"
//#include "utils/log.hpp"

namespace {
  auto dummy0 =
      eudaq::Factory<eudaq::Monitor>::Register<Mpw3Monitor, const std::string &,
                                               const std::string &>(
          Mpw3Monitor::m_id_factory);
}

Mpw3Monitor::Mpw3Monitor(const std::string &name, const std::string &runcontrol)
    : eudaq::Monitor(name, runcontrol) {

  mMpw3QtIf = new Mpw3QtIf();
}

Mpw3Monitor::~Mpw3Monitor() { delete mMpw3QtIf; }

void Mpw3Monitor::DoInitialise() {
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);

  mMpw3QtIf->annInit();
}

void Mpw3Monitor::DoConfigure() {
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  mEnPrint = conf->Get("ENABLE_PRINT", 0);
  mEnStdPrint = conf->Get("ENABLE_STD_PRINT", 0);
  mEnStdConverter = conf->Get("ENABLE_STD_CONVERTER", 1);
  mForwardToGui = conf->Get("FORWARD2GUI", 1);

  mMpw3QtIf->annConfig();
}

void Mpw3Monitor::DoStartRun() { mMpw3QtIf->annStartRun(); }

void Mpw3Monitor::DoStopRun() {}

void Mpw3Monitor::DoReset() {}

void Mpw3Monitor::DoTerminate() {}

void Mpw3Monitor::DoReceive(eudaq::EventSP ev) {

  //  printf("event received");
  //  return;
  if (mEnPrint)
    ev->Print(std::cout);
  if (mEnStdConverter) {
    //    return;
    auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(ev);
    if (!stdev) {
      // cast to StdEvent did not work, hopefully you provided a proper Raw2Std
      // Converter, as this is what we are trying to call now
      stdev = eudaq::StandardEvent::MakeShared();
      bool success =
          eudaq::StdEventConverter::Convert(ev, stdev, nullptr); // no conf

      if (success && stdev != nullptr) {
        if (mEnStdPrint)
          stdev->Print(std::cout);
        if (mForwardToGui)
          mMpw3QtIf->triggerEvt(stdev);
      } else {
        //        LOG(WARNING) <<
        std::cout << "ERROR converting MPW3 -> StdEvent";
      }
    }
  }
}

Mpw3QtIf *Mpw3Monitor::getMpw3QtIf() { return mMpw3QtIf; }
