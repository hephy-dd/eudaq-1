#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

#include <QDebug>
#include <QRegularExpression>

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

  auto atts = parseElogCfgLine(attributes);
  auto req = parseElogCfgLine(reqAtt);

  auto keys = ini->Keylist();
  QRegularExpression optionRegex(R"(Options (\w+))");
  QMap<QString, QStringList> options;
  for (const auto &k : keys) {
    auto match = optionRegex.match(k.c_str());
    if (!match.hasMatch()) {
      continue;
    }
    auto att = match.captured(1);

    auto attOpt = parseElogCfgLine(ini->Get(k, ""));
    options[att] = attOpt;
  }

  foreach (const auto &a, atts) {
    Attribute attribute;
    attribute.name = a;
    if (options.keys().contains(a)) {
      attribute.options = options[a];
    }
    attribute.required = req.contains(a) ? true : false;
    mAttributes << attribute;
    //    qDebug() << "added " << attribute.name << " with o " <<
    //    attribute.options
    //             << " req? " << attribute.required;
  }

  populateUi();

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

void ElogRunCtrl::populateUi() {
  int i = 0;
  foreach (const auto &att, mAttributes) {
    auto item = new QTableWidgetItem(att.name);
    ui->twAtt->setRowCount(i + 1);
    ui->twAtt->setItem(i, 0, item);
    i++;
  }
}

QStringList ElogRunCtrl::parseElogCfgLine(const std::string &key) {
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
