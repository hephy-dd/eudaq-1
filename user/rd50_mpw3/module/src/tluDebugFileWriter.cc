#include "defs.h"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StdEventConverter.hh"

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
  mOut << "Raw: trigger# TS-Begin TS-End; Std: t-Begin t-End\n";
}

void TluDbgFileWriter::WriteEvent(eudaq::EventSPC ev) {

  mOut << ev->GetTriggerN() << " " << ev->GetTimestampBegin() << " "
       << ev->GetTimestampEnd() << "; ";

  auto evstd = eudaq::StandardEvent::MakeShared();
  eudaq::StdEventConverter::Convert(ev, evstd, nullptr);
  mOut << evstd->GetTimeBegin() << " " << evstd->GetTimeEnd() << "\n";
  mOut.flush();
}
