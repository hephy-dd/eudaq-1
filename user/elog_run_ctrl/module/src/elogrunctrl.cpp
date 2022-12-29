#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

#include <QDebug>

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

void ElogRunCtrl::Initialise() {
  RunControl::Initialise();
  auto ini = GetInitConfiguration();
  auto attributes = ini->Get("Attributes", "");
  auto optType = ini->Get("Options Type", "");
  auto optCat = ini->Get("Options Category", "");
  auto reqAtt = ini->Get("Required Attributes", "");

  mAtt = parseElogConfig(attributes);
  mOptType = parseElogConfig(optType);
  mOptCat = parseElogConfig(optCat);
  mReqAtt = parseElogConfig(reqAtt);

  mElog.setElogProgramPath(ini->Get("elog_installation", "elog").c_str());
  mElog.setHost(ini->Get("elog_host", "localhost").c_str());
  mElog.setPort(ini->Get("elog_port", 8080));
  mElog.setLogbook(ini->Get("elog_logbook", "").c_str());
  //  mElog.debugPrint();
}

void ElogRunCtrl::Configure() { RunControl::Configure(); }

void ElogRunCtrl::StartRun() { RunControl::StartRun(); }

void ElogRunCtrl::StopRun() { RunControl::StopRun(); }

void ElogRunCtrl::Exec() { RunControl::Exec(); }

void ElogRunCtrl::Reset() {
  RunControl::Reset();
  mElog.reset();
}

void ElogRunCtrl::populateUi() {}

QStringList ElogRunCtrl::parseElogConfig(const std::string &key) {
  QString tmp(key.c_str());
  auto list = tmp.split(",");
  QStringList retval;
  for (auto &i : list) {
    // remove seperators
    i.remove(',');
    i = i.trimmed();
  }
  //  qDebug() << list;
  return list;
}
