#include "eudaq/FileDeserializer.hh"
#include "eudaq/FileReader.hh"

class Mpw3FileReader : public eudaq::FileReader {
public:
  Mpw3FileReader(const std::string &filename);
  eudaq::EventSPC GetNextEvent() override;

private:
  std::unique_ptr<eudaq::FileDeserializer> m_des;
  std::string m_filename;
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader,
                                                            std::string &>(
      eudaq::cstr2hash("mpw3raw"));
  auto dummy1 = eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader,
                                                            std::string &&>(
      eudaq::cstr2hash("mpw3raw"));
  // has to be same name as mpw3FileWriter file extension to be recognized by
  // euCliConverter
} // namespace

Mpw3FileReader::Mpw3FileReader(const std::string &filename)
    : m_filename(filename) {}

eudaq::EventSPC Mpw3FileReader::GetNextEvent() {
  if (!m_des) {
    m_des.reset(new eudaq::FileDeserializer(m_filename));
    if (!m_des)
      EUDAQ_THROW("unable to open file: " + m_filename);
  }
  eudaq::EventUP ev;
  uint32_t id;

  if (m_des->HasData()) {
    m_des->PreRead(id);
    ev =
        eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(id, *m_des);
    if (ev != nullptr) {
      std::cout << "read mpw3FrameEvent; blocks: " << ev->GetNumBlock()
                << " size [0] = " << ev->GetBlock(0).size() << "\n";
    }
    return std::move(ev);
  } else
    return nullptr;
}
