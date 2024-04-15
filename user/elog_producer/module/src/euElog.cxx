#include "eloggui.h"
#include "eudaq/OptionParser.hh"
#include <QApplication>
#include <iostream>

int main(int argc, char **argv) {
  QCoreApplication *qapp = new QApplication(argc, argv);

  eudaq::OptionParser op("EUDAQ Elog Producer", "2.0",
                         "The Elog Log Submitter");
  eudaq::Option<std::string> tname(op, "t", "tname", "", "string",
                                   "Runtime name of the eudaq application");
  eudaq::Option<std::string> rctrl(
      op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the run control to connect to");
  try {
    op.Parse(argv);
  } catch (...) {
    std::ostringstream err;
    return op.HandleMainException(err);
  }
  ElogGui gui(tname.Value(), rctrl.Value());
  try {
    gui.Connect(); // connecting to run control
  } catch (...) {
    std::cout << "Can not connect to RunControl at " << rctrl.Value()
              << std::endl;
    return -1;
  }

  //  while (prod.IsConnected()) {
  //    std::this_thread::sleep_for(std::chrono::seconds(1));
  //  }
  qApp->exec();

  return 0;
}
