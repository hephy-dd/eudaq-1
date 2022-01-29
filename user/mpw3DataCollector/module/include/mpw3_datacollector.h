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
  std::mutex mMtxMap;
  std::map<eudaq::ConnectionSPC, std::deque<eudaq::EventSPC>> mConnEvque;
  std::set<eudaq::ConnectionSPC> mConnInactive;

  std::unique_ptr<SVD::XLNX_CTRL::FADCGbEMerger> mEventMerger;
  std::unique_ptr<std::atomic<bool>> mEventBuilderRunning{};
  std::unique_ptr<std::thread> mEventBuilderThread{};
  std::vector<SVD::XLNX_CTRL::BackEndID_t> mBackEndIDs;
  eudaq::FileWriterSP mWriter;
};
#endif // MPW3_DATACOLLECTOR_H
