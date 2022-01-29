#include "mpw3_datacollector.h"

namespace {
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::Register<
      Mpw3FastDataCollector, const std::string &, const std::string &>(
      Mpw3FastDataCollector::m_id_factory);
}

Mpw3FastDataCollector::Mpw3FastDataCollector(const std::string &name,
                                             const std::string &rc)
    : DataCollector(name, rc) {}

void Mpw3FastDataCollector::DoConnect(eudaq::ConnectionSPC idx) {
  std::unique_lock<std::mutex> lk(mMtxMap);
  mConnEvque[idx].clear();
  mConnInactive.erase(idx);
}

void Mpw3FastDataCollector::DoDisconnect(eudaq::ConnectionSPC idx) {
  std::unique_lock<std::mutex> lk(mMtxMap);
  mConnInactive.insert(idx);
  if (mConnInactive.size() == mConnEvque.size()) {
    mConnInactive.clear();
    mConnEvque.clear();
  }
}

void Mpw3FastDataCollector::DoConfigure() {
  auto conf = GetConfiguration();
  if (conf) {
    conf->Print();

    SVD::XLNX_CTRL::BackEndID_t xlnxBoardId;
    xlnxBoardId.m_Address = conf->Get("VME_ADDR", 0);
    xlnxBoardId.m_Crate = conf->Get("CRATE_NO", 0);
    mBackEndIDs.push_back(xlnxBoardId);
  }
}

void Mpw3FastDataCollector::DoReset() {
  std::unique_lock<std::mutex> lk(mMtxMap);
  mConnEvque.clear();
  mConnInactive.clear();
}

void Mpw3FastDataCollector::DoStartRun() {

  mEventMerger = std::make_unique<SVD::XLNX_CTRL::FADCGbEMerger>(mBackEndIDs);

  mEventBuilderRunning = std::make_unique<std::atomic<bool>>(true);
  mEventBuilderThread = std::make_unique<std::thread>(
      &Mpw3FastDataCollector::WriteEudaqEventLoop, this);
}

void Mpw3FastDataCollector::DoStopRun() {
  mEventBuilderRunning->store(false, std::memory_order_release);
  mEventBuilderThread->join();
  mEventMerger.release();
}

void Mpw3FastDataCollector::DoReceive(eudaq::ConnectionSPC idx,
                                      eudaq::EventSP evsp) {
  /* we are no typical Eudaq DataCollector,
   * not collecting events from Producers!
   * This function would get called when a producer sent an event to us
   */
}

void Mpw3FastDataCollector::WriteEudaqEventLoop() {
  SVD::XLNX_CTRL::Event_t xlnxEvent;
  auto euEvent = eudaq::Event::MakeShared("CaribouRD50_MPW3Event");
  /*
   * I know we are actually no Caribou, but this way we can use the same
   * StandardEventConverter later on for UDP events and events from the
   * CaribouProducer
   */
  while (mEventBuilderRunning->load(std::memory_order_acquire)) {
    uint32_t eventN;
    uint32_t triggerN;

    if ((*mEventMerger)(xlnxEvent)) {
      // simply put data in event, StandardEventConverter got time to extract
      // triggerNr, pixelHit,...
      euEvent->AddBlock(0, xlnxEvent.m_Data);
      WriteEvent(euEvent);

    } else {
      continue;
    }
  }
}
