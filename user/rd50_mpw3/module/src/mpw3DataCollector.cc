#include "../tools/Defs.h"
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
    xlnxBoardId.m_Address = conf->Get("VME_ADDR", 1);
    xlnxBoardId.m_Crate = conf->Get("CRATE_NO", 1);
    mBackEndIDs.push_back(xlnxBoardId);
  }
}

void Mpw3FastDataCollector::DoInitialise() {}

void Mpw3FastDataCollector::DoReset() {
  std::unique_lock<std::mutex> lk(mMtxMap);
  mConnEvque.clear();
  mConnInactive.clear();
}

void Mpw3FastDataCollector::DoStartRun() {

  mEventMerger = std::make_unique<SVD::XLNX_CTRL::FADCGbEMerger>(
      mBackEndIDs, eudaq::GetLogger());

  mEventBuilderRunning = std::make_unique<std::atomic<bool>>(true);
  mEventBuilderThread = std::make_unique<std::thread>(
      &Mpw3FastDataCollector::WriteEudaqEventLoop, this);

  mStartTime = std::chrono::high_resolution_clock::now();
}

void Mpw3FastDataCollector::DoStopRun() {
  mEventBuilderRunning->store(false, std::memory_order_release);
  mEventBuilderThread->join();
  mEventBuilderThread.reset();
  mEventBuilderRunning.reset();
  mEventMerger.reset();
}

void Mpw3FastDataCollector::DoReceive(eudaq::ConnectionSPC idx,
                                      eudaq::EventSP evsp) {
  /* we are no typical Eudaq DataCollector,
   * not collecting events from Producers!
   * This function would get called when a producer sent an event to us
   */
}

void Mpw3FastDataCollector::WriteEudaqEventLoop() {
  SVD::XLNX_CTRL::Event_t frame;
  uint32_t nEuEvent = 0;
  uint32_t idleLoops = 0;
  std::chrono::high_resolution_clock::time_point lastTime =
      std::chrono::high_resolution_clock::now();
  auto euEvent = eudaq::Event::MakeShared("Mpw3FrameEvent");

  while (mEventBuilderRunning->load(std::memory_order_acquire)) {
    if ((*mEventMerger)(frame)) {
      // simply put data in event, StandardEventConverter got time to extract
      // triggerNr, pixelHit,...

      //      if (nEuEvent++ % 1000 == 0) {
      //        auto duration = std::chrono::high_resolution_clock::now() -
      //        lastTime; std::cout << "writing euEvent " << nEuEvent << " perf
      //        = "
      //                  << double(32 * frame.m_Data.front().size()) * 1e5 /
      //                         (double(duration.count()) * 1e-9)
      //                  << " Bit/s \n";

      //        lastTime = std::chrono::high_resolution_clock::now();
      //      }
      for (int i = 0; i < frame.m_Data.size(); i++) {
        if (frame.m_Data[i].size() > 2) {
          euEvent->AddBlock(i, frame.m_Data[i]);
          euEvent->SetEventN(frame.m_EventNr);
          WriteEvent(euEvent);
        } else {
          EUDAQ_WARN("too small frame size <= 2");
        }
        // there might be more than 1 unpacker
        // assigned to this merger
      }
    } else {
      idleLoops++;
      continue;
    }
  }
}
