#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

namespace {
auto dummy0 = eudaq::Factory<eudaq::RunControl>::Register<ElogRunCtrl,
                                                          const std::string &>(
    ElogRunCtrl::m_id_factory);
}

ElogRunCtrl::ElogRunCtrl(const std::string &listenaddress, QWidget *parent)
    : QWidget(parent), eudaq::RunControl(listenaddress),
      ui(new Ui::ElogRunCtrl) {
  ui->setupUi(this);
  this->show();
}

ElogRunCtrl::~ElogRunCtrl() { delete ui; }

void ElogRunCtrl::Initialise()
{

}

void ElogRunCtrl::Configure()
{

}

void ElogRunCtrl::StartRun()
{

}

void ElogRunCtrl::StopRun()
{

}

void ElogRunCtrl::Exec()
{

}

void ElogRunCtrl::Reset()
{

}
