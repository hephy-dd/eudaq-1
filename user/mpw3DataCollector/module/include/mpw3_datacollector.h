#ifndef MPW3_DATACOLLECTOR_H
#define MPW3_DATACOLLECTOR_H

#include "Event_t.h"
#include "XilinxEudaqInterface.h"
#include "eudaq/DataCollector.hh"

#include <deque>
#include <map>
#include <mutex>
#include <set>

class Mpw3FastDataCollector : public eudaq::DataCollector {
public:
  Mpw3FastDataCollector(const std::string &name, const std::string &rc);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoConfigure() override;
  void DoReset() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("MPW3DataCollector");

private:
  void WriteEudaqEventLoop();
  void dummyDataGenerator();
  std::mutex mMtxMap;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> mConnEvque;
  std::set<eudaq::ConnectionSPC> mConnInactive;

  std::unique_ptr<SVD::XLNX_CTRL::FADCGbEMerger> mEventMerger;
  std::unique_ptr<std::atomic<bool>> mEventBuilderRunning{};
  std::unique_ptr<std::atomic<bool>> mTestRunning{};
  std::unique_ptr<std::thread> mEventBuilderThread{};
  std::unique_ptr<std::thread> mTestThread{};
  std::vector<SVD::XLNX_CTRL::BackEndID_t> mBackEndIDs;
  eudaq::FileWriterSP mWriter;
  std::shared_ptr<SVD::XLNX_CTRL::UPDDetails::PayloadBuffer_t> mTestBuffer{};
  std::chrono::high_resolution_clock::time_point mStartTime;
};
#endif // MPW3_DATACOLLECTOR_H
