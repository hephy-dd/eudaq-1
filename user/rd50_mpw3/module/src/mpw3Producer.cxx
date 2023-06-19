#include "eudaq/Configuration.hh"
#include "eudaq/Producer.hh"

#include "device/DeviceManager.hpp"
#include "utils/configuration.hpp"
#include "utils/log.hpp"

#include <thread>
#include <vector>

using namespace caribou;

class Mpw3Producer : public eudaq::Producer {
public:
  Mpw3Producer(const std::string name, const std::string &runcontrol);
  ~Mpw3Producer() override;
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoReset() override;
  // void DoTerminate() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Mpw3Producer");

private:
  unsigned mEvtNmb;

  DeviceManager *mDevManager;
  std::vector<Device *> mDevices;
  std::string mName;

  std::mutex mDevMutex;
  LogLevel mLogLvl;
  bool mExitOfRun;
  bool mPiggyAttached;
};

namespace {
auto dummy0 =
    eudaq::Factory<eudaq::Producer>::Register<Mpw3Producer, const std::string &,
                                              const std::string &>(
        Mpw3Producer::m_id_factory);
}

Mpw3Producer::Mpw3Producer(const std::string name,
                           const std::string &runcontrol)
    : eudaq::Producer(name, runcontrol), mEvtNmb(0), mExitOfRun(false),
      mName(name) {
  // Add cout as the default logging stream
  Log::addStream(std::cout);

  LOG(INFO) << "Instantiated Mpw3Producer for device \"" << name << "\"";

  // Create new Peary device manager
  mDevManager = new DeviceManager();
}

Mpw3Producer::~Mpw3Producer() { delete mDevManager; }

void Mpw3Producer::DoReset() {
  LOG(WARNING) << "Resetting CaribouProducer";
  mExitOfRun = true;

  // Delete all devices:
  std::lock_guard<std::mutex> lock{mDevMutex};
  mDevManager->clearDevices();
}

void Mpw3Producer::DoInitialise() {
  LOG(INFO) << "Initialising Mpw3Producer";
  auto ini = GetInitConfiguration();

  std::istringstream devicesStr(ini->Get("devices", std::string()));
  std::string tmp;
  std::vector<std::string> devices;

  for (std::string tmp; std::getline(devicesStr, tmp, ',');) {

    devices.push_back(tmp);
  }

  auto level = ini->Get("log_level", "INFO");
  try {
    LogLevel log_level = Log::getLevelFromString(level);
    Log::setReportingLevel(log_level);
    LOG(INFO) << "Set verbosity level to \"" << std::string(level) << "\"";
  } catch (std::invalid_argument &e) {
    LOG(ERROR) << "Invalid verbosity level \"" << std::string(level)
               << "\", ignoring.";
  }

  mLogLvl = Log::getReportingLevel();

  // Open configuration file and create object:
  caribou::Configuration config;
  auto confname = ini->Get("config_file", "");
  std::ifstream file(confname);
  EUDAQ_INFO("Attempting to use initial device configuration \"" + confname +
             "\"");
  if (!file.is_open()) {
    LOG(ERROR) << "No configuration file provided.";
    EUDAQ_ERROR("No Caribou configuration file provided.");
  } else {
    config = caribou::Configuration(file);
  }

  // Select section from the configuration file relevant for this device:
  //  auto sections = config.GetSections();

  std::lock_guard<std::mutex> lock{mDevMutex};

  auto sections = config.GetSections();
  for (const auto &d : devices) {
    if (std::find(sections.begin(), sections.end(), d) != sections.end()) {
      config.SetSection(d);
    } else {
      EUDAQ_WARN("no config section found for device " + d +
                 " in Caribou config file");
    }
    auto id = mDevManager->addDevice(d, config);
    EUDAQ_INFO("Manager returned device ID " + std::to_string(id) +
               ", for device " + d);
    mDevices.push_back(mDevManager->getDevice(id));
  }
}

// This gets called whenever the DAQ is configured
void Mpw3Producer::DoConfigure() {
  auto config = GetConfiguration();
  LOG(INFO) << "Configuring CaribouProducer: " << config->Name();

  std::lock_guard<std::mutex> lock{mDevMutex};

  for (auto dev : mDevices) {
    EUDAQ_INFO("Configuring device " + dev->getName());
    dev->powerOn();
    // Wait for power to stabilize and for the TLU clock to be present
    eudaq::mSleep(1000);

    dev->configure();
  }

  LOG(STATUS) << "Mpw3Producer configured. Ready to start run.";
}

void Mpw3Producer::DoStartRun() {
  mEvtNmb = 0;

  LOG(INFO) << "Starting run...";

  // Start the DAQ
  std::lock_guard<std::mutex> lock{mDevMutex};

  // Sending initial Begin-of-run event, just containing tags with detector
  // information: Create new event
  auto event = eudaq::Event::MakeUnique("Caribou" + mName + "Event");
  event->SetBORE();
  for (auto dev : mDevices) {
    event->SetTag(dev->getName() + " software", dev->getVersion());
    event->SetTag(dev->getName() + "firmware", dev->getFirmwareVersion());
    event->SetTag(dev->getName() + "timestamp", LOGTIME);

    auto registers = dev->getRegisters();
    for (const auto &reg : registers) {
      auto regName = dev->getName() + ":" + reg.first;
      event->SetTag(regName, reg.second);
    }
  }

  // Send the event to the Data Collector
  SendEvent(std::move(event));

  // Start DAQ:
  for (auto dev : mDevices) {
    dev->daqStart();
  }

  LOG(INFO) << "Started run.";
  mExitOfRun = false;
}

void Mpw3Producer::DoStopRun() {

  LOG(INFO) << "Stopping run...";

  // Set a flag to signal to the polling loop that the run is over
  mExitOfRun = true;

  // Stop the DAQ
  std::lock_guard<std::mutex> lock{mDevMutex};
  for (auto dev : mDevices) {
    dev->daqStop();
  }
  LOG(INFO) << "Stopped run.";
}

void Mpw3Producer::RunLoop() {

  Log::setReportingLevel(mLogLvl);

  LOG(INFO) << "Starting run loop...";
  std::lock_guard<std::mutex> lock{mDevMutex};
  while (!mExitOfRun) {
    try {
      int devNmb = 0;
      bool gotData = false;
      eudaq::EventUP event = nullptr;
      for (auto dev : mDevices) {
        auto data = dev->getRawData();
        if (!data.empty()) {
          gotData = true;
          // Create new event
          if (event == nullptr) {
            event = eudaq::Event::MakeUnique("Caribou" + mName + "Event");
            // Set event ID
            event->SetEventN(mEvtNmb);
          }
          event->AddBlock(devNmb, data);
        }
        devNmb++;
      }
      if (gotData) {
        SendEvent(std::move(event));
        mEvtNmb++;
      }

      LOG_PROGRESS(STATUS, "status") << "Frame " << mEvtNmb;
    } catch (caribou::NoDataAvailable &) {
      continue;
    } catch (caribou::DataException &e) {
      // Retrieval failed, retry once more before aborting:
      EUDAQ_WARN(std::string(e.what()) + ", skipping data packet");
      continue;
    } catch (caribou::caribouException &e) {
      EUDAQ_ERROR(e.what());
      break;
    }
  }

  LOG(INFO) << "Exiting run loop.";
}
