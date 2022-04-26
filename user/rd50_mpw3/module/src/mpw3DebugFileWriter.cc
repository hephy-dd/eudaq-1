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
      eudaq::cstr2hash("txt"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::Register<Mpw3DbgFileWriter,
                                                            std::string &&>(
      eudaq::cstr2hash("txt"));
} // namespace

Mpw3DbgFileWriter::Mpw3DbgFileWriter(const std::string &patt) {
  m_filepattern = patt;
  mOut = std::ofstream(patt);
  mOut << "#trigger# TS FTS0 FTS1 FTS2 FTS3 FTS4 FTS5 type\n";
}

void Mpw3DbgFileWriter::WriteEvent(eudaq::EventSPC ev) {
  //  const std::vector<uint32_t> validData = {0xaf0000cc, 0x101f0603,
  //  0x11200603,
  //                                           0x1120130f, 0x12220603,
  //                                           0x1223130f, 0x12241917,
  //                                           0x13250603, 0xe00000cc};
  //  auto block = ev->GetBlock(0);
  //  std::vector<uint32_t> data;
  //  const auto wordSize = sizeof(uint32_t);

  //  data.resize(block.size() / wordSize);

  //  memcpy(data.data(), block.data(), block.size());

  //  static int wordCnt = 0;
  //  for (auto i = data.begin(); i < data.end(); i++) {
  //    wordCnt++;
  //    if (DefsMpw3::extractPiggy(*i) == 0) {
  //      mOut << std::hex << *i << "\n";
  //      if (std::count(validData.begin(), validData.end(), *i) == 0) {
  //        std::cout << "invalid word# " << wordCnt << ": " << std::hex << *i
  //                  << std::dec << "\n";
  //      }
  //    }
  //  }

  mOut << ev->GetTriggerN() << " " << ev->GetTimestampBegin() << " "
       << " " << ev->GetTag("TRIGGER") << " " << ev->GetTag("FINE_TS0") << " "
       << ev->GetTag("FINE_TS1") << " " << ev->GetTag("FINE_TS2") << " "
       << ev->GetTag("FINE_TS3") << " " << ev->GetTag("FINE_TS4") << " "
       << ev->GetTag("FINE_TS5") << " " << ev->GetTag("TYPE") << "\n";
  mOut.flush();
}
