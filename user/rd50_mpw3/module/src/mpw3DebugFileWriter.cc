#include "defs.h"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StdEventConverter.hh"

#define PROCESS_PREPROCESSED

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

#ifndef PROCESS_PREPROCESSED

  static int evtCnt = 0;

  mOut << "\n\n new event #" << evtCnt++
       << " received @ FW-TS = " << ev->GetTag("recvTS_FW")
       << " CPU TS = " << ev->GetTag("recvTS_CPU") << "\n\n";

  auto block = ev->GetBlock(0);
  std::vector<uint32_t> data;
  const auto wordSize = sizeof(uint32_t);

  data.resize(block.size() / wordSize);

  memcpy(data.data(), block.data(), block.size());

  static int wordCnt = 0;
  for (auto i : data) {
    wordCnt++;
    DefsMpw3::HitInfo hi(i);
    mOut << std::hex << i << " " << std::dec << hi.toStr() << "\n";
  }
#else
  auto evstd = eudaq::StandardEvent::MakeShared();
  auto success = eudaq::StdEventConverter::Convert(ev, evstd, nullptr);
  if (evstd == nullptr || !success) {
    std::cout << "error during event conversion\n";
    return;
  }
  mOut << "\n"
       << evstd->GetTimestampBegin() << " => t = " << evstd->GetTimeBegin()
       << " multiple? " << ev->GetTag("hitInMultipleEvents", false) << " |  ";
  auto plane = evstd->GetPlane(0);
  for (int i = 0; i < plane.HitPixels(); i++) {
    auto col = int(plane.GetX(i));
    auto row = int(plane.GetY(i));
    auto tot = int(plane.GetPixel(i));
    mOut << row << ":" << col << "/" << tot << ", ";
  }
#endif
  mOut.flush();
}
