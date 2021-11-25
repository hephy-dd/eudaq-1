#ifndef MPW3EVENTCONVERTER_H
#define MPW3EVENTCONVERTER_H
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3RawEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory =
      eudaq::cstr2hash("CaribouRD50_MPW2Event");
};

#endif // MPW3EVENTCONVERTER_H
