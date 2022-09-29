#include "defs.h"
#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"

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
}

void Mpw3DbgFileWriter::WriteEvent(eudaq::EventSPC ev) {
  std::cout << "new event\n";
  mOut << "\n\n new event\n\n";
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
  mOut.flush();
}
