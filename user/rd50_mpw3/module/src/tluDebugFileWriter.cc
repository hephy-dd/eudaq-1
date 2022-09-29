#include "defs.h"
#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"

class TluDbgFileWriter : public eudaq::FileWriter {
public:
  TluDbgFileWriter(const std::string &patt);
  void WriteEvent(eudaq::EventSPC ev) override;

private:
  std::unique_ptr<eudaq::FileSerializer> m_ser;
  std::string m_filepattern;
  uint32_t m_run_n;
  std::ofstream mOut = {};
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileWriter>::Register<TluDbgFileWriter,
                                                            std::string &>(
      eudaq::cstr2hash("tlutxt"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::Register<TluDbgFileWriter,
                                                            std::string &&>(
      eudaq::cstr2hash("tlutxt"));
} // namespace

TluDbgFileWriter::TluDbgFileWriter(const std::string &patt) {
  m_filepattern = patt;
  mOut = std::ofstream(patt);
  mOut << "#trigger# TS Trigger BORE EORE FTS0 FTS1 FTS2 FTS3 FTS4 FTS5 type "
          "particles "
          "scal0 scal1 scal2 scal3 scal4 scal5\n";
}

void TluDbgFileWriter::WriteEvent(eudaq::EventSPC ev) {

  mOut << ev->GetTriggerN() << " " << ev->GetTimestampBegin() << " "
       << " " << ev->IsBORE() << " " << ev->IsEORE() << " "
       << ev->GetTag("TRIGGER") << " " << ev->GetTag("FINE_TS0") << " "
       << ev->GetTag("FINE_TS1") << " " << ev->GetTag("FINE_TS2") << " "
       << ev->GetTag("FINE_TS3") << " " << ev->GetTag("FINE_TS4") << " "
       << ev->GetTag("FINE_TS5") << " " << ev->GetTag("TYPE")
       << ev->GetTag("PARTICLES") << " " << ev->GetTag("SCALER0") << " "
       << ev->GetTag("SCALER1") << " " << ev->GetTag("SCALER2") << " "
       << ev->GetTag("SCALER3") << " " << ev->GetTag("SCALER4") << " "
       << ev->GetTag("SCALER5") << "\n";
  mOut.flush();
}
