#include "CaribouEvent2StdEventConverter.hh"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Mpw3RawEvent2StdEventConverter>(
      Mpw3RawEvent2StdEventConverter::m_id_factory);
}

bool Mpw3RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1,
                                                eudaq::StdEventSP d2,
                                                eudaq::ConfigSPC conf) const {

  /* this event contains the very same data-format as the ones sent by the
   * MPW3DataCollector the event type is hard coded within CaribouProducer,
   * therefor we need an extra event-converter Use this simple hack to not have
   * to copy the code from "Mpw3FrameEvent2StdEventConverter"
   *
   * !!! Attention:
   * Eudaq has to be built with the RD50_MPW3 user-module in order for this to
   * work
   */
  auto event = eudaq::Event::MakeShared("Mpw3FrameEvent");
  // Set event ID
  event->SetEventN(d1->GetEventN());
  // Add data to the event
  event->AddBlock(0, d1->GetBlock(0));

  auto retval = eudaq::StdEventConverter::Convert(event, d2, nullptr);
  std::cout << " did work? " << retval;
  return retval;
}
