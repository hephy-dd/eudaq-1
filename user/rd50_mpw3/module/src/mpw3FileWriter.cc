#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/FileWriter.hh"

class Mpw3FileWriter : public eudaq::FileWriter {
public:
  Mpw3FileWriter(const std::string &patt);
  void WriteEvent(eudaq::EventSPC ev) override;
  uint64_t FileBytes() const override;

private:
  std::unique_ptr<eudaq::FileSerializer> m_ser;
  std::string m_filepattern;
  uint32_t m_run_n;
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileWriter>::Register<Mpw3FileWriter,
                                                            std::string &>(
      eudaq::cstr2hash("mpw3_fw"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::Register<Mpw3FileWriter,
                                                            std::string &&>(
      eudaq::cstr2hash("mpw3_fw"));
} // namespace

Mpw3FileWriter::Mpw3FileWriter(const std::string &patt) {
  m_filepattern = patt;
}

void Mpw3FileWriter::WriteEvent(eudaq::EventSPC ev) {
  uint32_t run_n = ev->GetRunN();
  if (!m_ser || m_run_n != run_n) {
    std::time_t time_now = std::time(nullptr);
    char time_buff[13];
    time_buff[12] = 0;
    std::strftime(time_buff, sizeof(time_buff), "%y%m%d%H%M%S",
                  std::localtime(&time_now));
    std::string time_str(time_buff);
    m_ser.reset(new eudaq::FileSerializer((eudaq::FileNamer(m_filepattern)
                                               .Set('X', ".mpw3raw")
                                               .Set('R', run_n)
                                               .Set('D', time_str))));
    m_run_n = run_n;
  }
  if (!m_ser)
    EUDAQ_THROW("NativeFileWriter: Attempt to write unopened file");
  m_ser->write(*(ev.get())); // TODO: Serializer accepts EventSPC
  m_ser->Flush();
}

uint64_t Mpw3FileWriter::FileBytes() const {
  return m_ser ? m_ser->FileBytes() : 0;
}
