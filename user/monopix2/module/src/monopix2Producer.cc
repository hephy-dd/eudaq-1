#include "eudaq/Producer.hh"
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <ratio>
#include <thread>
#ifndef _WIN32
#include <sys/file.h>
#endif

class Monopix2Producer : public eudaq::Producer {
public:
  Monopix2Producer(const std::string &name, const std::string &runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Monopix2Producer");

private:
  FILE *mLockFile;
  std::atomic_flag mRunLoop;
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<
      Monopix2Producer, const std::string &, const std::string &>(
      Monopix2Producer::m_id_factory);
}

Monopix2Producer::Monopix2Producer(const std::string &name,
                                   const std::string &runcontrol)
    : eudaq::Producer(name, runcontrol), mLockFile(0), mRunLoop(false) {}

void Monopix2Producer::DoInitialise() {
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("DEV_LOCK_PATH", "monopix2lockfile.txt");
  mLockFile = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if (flock(fileno(mLockFile), LOCK_EX | LOCK_NB)) { // fail
    EUDAQ_THROW("unable to lock the lockfile: " + lock_path);
  }
#endif
}

void Monopix2Producer::DoConfigure() {
  auto conf = GetConfiguration();
  conf->Print(std::cout);

  auto dummy1 = conf->Get("DUMMY1", "");
  auto dummy2 = conf->Get("DUMMY2", "");

  EUDAQ_DEBUG("config dummies: " + dummy1 + " " + dummy2);
}

void Monopix2Producer::DoStartRun() { mRunLoop.test_and_set(); }

void Monopix2Producer::DoStopRun() { mRunLoop.clear(); }

void Monopix2Producer::DoReset() {
  mRunLoop.clear();
  if (mLockFile) {
#ifndef _WIN32
    flock(fileno(mLockFile), LOCK_UN);
#endif
    fclose(mLockFile);
    mLockFile = 0;
  }
}

void Monopix2Producer::DoTerminate() {
  mRunLoop.clear();
  if (mLockFile) {
    fclose(mLockFile);
    mLockFile = 0;
  }
}

void Monopix2Producer::RunLoop() {

  while (mRunLoop.test_and_set()) {
    auto ev = eudaq::Event::MakeUnique("Monopix2Raw");
    SendEvent(std::move(ev));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
