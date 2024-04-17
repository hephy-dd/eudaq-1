#include "defs.h"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3DbgFileWriter : public eudaq::FileWriter {
public:
  Mpw3DbgFileWriter(const std::string &patt);
  void WriteEvent(eudaq::EventSPC ev) override;

private:
  std::unique_ptr<eudaq::FileSerializer> m_ser;
  std::string m_filepattern;
  uint32_t m_run_n;
  std::ofstream mOut = {};
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileWriter>::Register<Mpw3DbgFileWriter,
                                                            std::string &>(
      eudaq::cstr2hash("mpw3txt"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::Register<Mpw3DbgFileWriter,
                                                            std::string &&>(
      eudaq::cstr2hash("mpw3txt"));
} // namespace

Mpw3DbgFileWriter::Mpw3DbgFileWriter(const std::string &patt) {
  m_filepattern = patt;
  mOut = std::ofstream(patt);
  mOut << "# TS |   pixel/tot; pixel/tot, ...\n";
}

void Mpw3DbgFileWriter::WriteEvent(eudaq::EventSPC ev) {

  static int evtCnt = 0;

  mOut << "\n\n new event #" << evtCnt++ << "type = " << ev->GetTag("Type")
       << " payloadId = " << ev->GetTag("payloadID") << "\n\n";

  auto blockIdx = ev->GetTag("Type") == "Base" ? 0 : 1;

  auto block = ev->GetBlock(blockIdx);

  std::vector<uint32_t> data;
  const auto wordSize = sizeof(uint32_t);

  if (block.size() <= 2 * wordSize) {
    // no hits in event just trigger word,
    // actually not usable, but don't fail for debug purposes
    mOut << "empty\n\n";
  }

  data.resize(block.size() / wordSize);

  memcpy(data.data(), block.data(), block.size());

  static int wordCnt = 0;
  DefsMpw3::word_t sof, eof;
  for (auto i : data) {
    wordCnt++;
    DefsMpw3::HitInfo hi(i);
    mOut << std::hex << i << " " << std::dec << hi.toStr() << "\n"
         << std::setprecision(10);
  }
  double ovflwT = 0, tluT = 0;
  eudaq::Configuration cfg;
  cfg.Set("ts_mode", "Ovflw");
  cfg.Set("lsb_time", 25000);
  //  const eudaq::Configuration eu_cfg = cfg;
  auto eudaq_config = std::make_shared<const eudaq::Configuration>(cfg);
  auto evstd = eudaq::StandardEvent::MakeShared();
  auto success = eudaq::StdEventConverter::Convert(ev, evstd, eudaq_config);
  if (evstd == nullptr || !success) {
    mOut << "\nStdEvent conversion failed";
    mOut.flush();
    //    std::cout << "error during event conversion\n";
    //      return;
  } else {
    ovflwT = evstd->GetTimeBegin() * 1e-6;
    mOut << "\nStdEvent: Ovflw t = " << ovflwT << "us";
  }

  cfg.Set("ts_mode", "TLU");
  cfg.Set("lsb_time", 25000);
  //  const eudaq::Configuration eu_cfg = cfg;
  eudaq_config = std::make_shared<const eudaq::Configuration>(cfg);
  evstd = eudaq::StandardEvent::MakeShared();
  success = eudaq::StdEventConverter::Convert(ev, evstd, eudaq_config);
  if (evstd == nullptr || !success) {
    mOut << "\nStdEvent conversion TLU time failed";
    mOut.flush();
    //    std::cout << "error during event conversion\n";
    //      return;
  } else {
    tluT = evstd->GetTimeBegin() * 1e-6;
    mOut << "\nStdEvent: TLU t = " << tluT << "us";
  }
  mOut << "\ndiff = " << ovflwT - tluT << "us";

  //  auto plane = evstd->GetPlane(0);
  //  for (int i = 0; i < plane.HitPixels(); i++) {
  //    auto col = int(plane.GetX(i));
  //    auto row = int(plane.GetY(i));
  //    auto tot = int(plane.GetPixel(i));
  //    mOut << row << ":" << col << "/" << tot << ", ";
  //  }

  mOut.flush();
}
