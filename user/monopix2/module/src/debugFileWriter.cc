#include "eudaq/Event.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Monopix2DbgFileWriter : public eudaq::FileWriter {
public:
  Monopix2DbgFileWriter(const std::string &patt);
  void WriteEvent(eudaq::EventSPC ev) override;

private:
  std::unique_ptr<eudaq::FileSerializer> m_ser;
  std::string m_filepattern;
  uint32_t m_run_n;
  std::ofstream mOut = {};
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileWriter>::Register<
      Monopix2DbgFileWriter, std::string &>(eudaq::cstr2hash("txt"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::Register<
      Monopix2DbgFileWriter, std::string &&>(eudaq::cstr2hash("txt"));
} // namespace

Monopix2DbgFileWriter::Monopix2DbgFileWriter(const std::string &patt) {
  m_filepattern = patt;
  mOut = std::ofstream(patt);
  mOut << "#col row tot TS trgNmb trgTs\n";
}

void Monopix2DbgFileWriter::WriteEvent(eudaq::EventSPC ev) {
  auto evstd = eudaq::StandardEvent::MakeShared();
  if (!eudaq::StdEventConverter::Convert(ev, evstd, nullptr)) {
    std::cout << "conversion failed\n";
    return; // conversion failed
  }

  auto plane = evstd->GetPlane(0);
  for (int i = 0; i < plane.HitPixels(); i++) {
    auto col = plane.GetX(i);
    auto row = plane.GetY(i);
    auto tot = plane.GetPixel(i);
    auto triggerNmb = evstd->GetTriggerN();

    mOut << col << " " << row << " " << tot << " " << evstd->GetTimestampBegin()
         << " " << triggerNmb << " " << evstd->GetTag("trgTs") << "\n";
  }
}
