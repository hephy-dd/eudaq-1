#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Ex0RawEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Mpw3FrameEvent");

private:
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Ex0RawEvent2StdEventConverter>(
      Ex0RawEvent2StdEventConverter::m_id_factory);
}

bool Ex0RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1,
                                               eudaq::StdEventSP d2,
                                               eudaq::ConfigSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks = ev->NumBlocks();

  return true;
}
