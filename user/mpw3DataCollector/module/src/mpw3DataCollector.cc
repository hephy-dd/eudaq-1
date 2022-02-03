#include "mpw3_datacollector.h"

namespace {
  auto dummy0 = eudaq::Factory<eudaq::DataCollector>::Register<
      Mpw3FastDataCollector, const std::string &, const std::string &>(
      Mpw3FastDataCollector::m_id_factory);
}

Mpw3FastDataCollector::Mpw3FastDataCollector(const std::string &name,
                                             const std::string &rc)
    : DataCollector(name, rc),
      mTestBuffer(
          std::make_shared<SVD::XLNX_CTRL::UPDDetails::PayloadBuffer_t>()) {}

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

  mEventMerger =
      std::make_unique<SVD::XLNX_CTRL::FADCGbEMerger>(mBackEndIDs, mTestBuffer);

  mEventBuilderRunning = std::make_unique<std::atomic<bool>>(true);
  mEventBuilderThread = std::make_unique<std::thread>(
      &Mpw3FastDataCollector::WriteEudaqEventLoop, this);
  mTestThread =
      std::make_unique<std::thread>(&Mpw3FastDataCollector::testLoop, this);
  mTestRunning = std::make_unique<std::atomic<bool>>(true);
}

void Mpw3FastDataCollector::DoStopRun() {
  mEventBuilderRunning->store(false, std::memory_order_release);
  mEventBuilderThread->join();
  mEventMerger.release();

  mTestRunning->store(false, std::memory_order_release);
  mTestThread->join();
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
  uint32_t nEuEvent = 0;
  auto euEvent = eudaq::Event::MakeShared("CaribouRD50_MPW3Event");
  /*
   * I know we are actually no Caribou, but this way we can use the same
   * StandardEventConverter later on for UDP events and events from the
   * CaribouProducer
   */
  while (mEventBuilderRunning->load(std::memory_order_acquire)) {
    if ((*mEventMerger)(xlnxEvent)) {
      // simply put data in event, StandardEventConverter got time to extract
      // triggerNr, pixelHit,...

      if (nEuEvent++ % 100 == 0) {
        std::cout << "writing euEvent #" << nEuEvent << "\n" << std::flush;
      }
      euEvent->AddBlock(0, xlnxEvent.m_Data);
      WriteEvent(euEvent);

    } else {
      continue;
    }
  }
}

void Mpw3FastDataCollector::testLoop() {
  const uint32_t head = 0xC << 28;
  const uint32_t tail = 0xE << 28;
  uint32_t packageNmb = 0;
  SVD::XLNX_CTRL::UPDDetails::Payload_t data;
  while (mTestRunning->load(std::memory_order_acquire)) {
    data.clear();
    packageNmb++;
    if (packageNmb % 100 == 0) {
      std::cout << "sending pack " << packageNmb << "\n" << std::flush;
    }
    data.push_back(head);
    for (int i = 0; i < 16; i++) {
      data.push_back(std::rand());
    }
    data.push_back(tail);
    data.push_back(packageNmb << 8);
    mTestBuffer->Push(data);
    std::this_thread::sleep_for(std::chrono::nanoseconds(10));
  }
}
