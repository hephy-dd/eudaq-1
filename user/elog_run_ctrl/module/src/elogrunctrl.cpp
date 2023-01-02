#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

#include <QComboBox>
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
  connect(ui->pbSubmit, &QPushButton::clicked, this, &ElogRunCtrl::submit);
}

ElogRunCtrl::~ElogRunCtrl() { delete ui; }

void ElogRunCtrl::Initialise() {
  RunControl::Initialise();

  elogSetup();
  populateUi();
}

void ElogRunCtrl::Configure() { RunControl::Configure(); }

void ElogRunCtrl::StartRun() { RunControl::StartRun(); }

void ElogRunCtrl::StopRun() { RunControl::StopRun(); }

void ElogRunCtrl::Exec() { RunControl::Exec(); }

void ElogRunCtrl::Reset() {
  RunControl::Reset();
  mElog.reset();
}

void ElogRunCtrl::submit() {
  using Att = QPair<QString, QString>;
  QList<Att> attributes;
  for (int i = 0; i < ui->twAtt->rowCount(); i++) {
    Att att;
    auto nameItem = ui->twAtt->item(i, 0);
    if (nameItem == nullptr) {
      qWarning() << "trying to send uninitialized attribute name";
      continue;
    }
    att.first = nameItem->data(Qt::DisplayRole).toString();
    auto valItem = ui->twAtt->item(i, 1);
    if (valItem != nullptr) {
      att.second = valItem->data(Qt::DisplayRole).toString();
    }
    auto w = ui->twAtt->cellWidget(i, 1);
    auto cb = qobject_cast<QComboBox *>(w);
    if (cb != nullptr) {
      att.second = cb->currentText();
    }
    attributes << att;
  }
  mElog.submitEntry(attributes, ui->teMessage->toPlainText());
}

void ElogRunCtrl::populateUi() {
  int i = 0;
  foreach (const auto &att, mAttributes) {
    auto item = new QTableWidgetItem(att.name);
    ui->twAtt->setRowCount(i + 1);
    ui->twAtt->setItem(i, 0, item);
    if (!att.options.isEmpty()) {
      auto cb = new QComboBox;
      ui->twAtt->setCellWidget(i, 1, cb);
      foreach (const auto &o, att.options) {
        cb->addItem(o);
      }
    }
    i++;
  }
}

void ElogRunCtrl::elogSetup() {
  auto ini = GetInitConfiguration();
  auto attributes = ini->Get("Attributes", "");
  auto reqAtt = ini->Get("Required Attributes", "");

  ui->twAtt->setRowCount(0);
  mAttributes.clear();

  auto atts = parseElogCfgLine(attributes);
  auto req = parseElogCfgLine(reqAtt);

  auto keys = ini->Keylist();

  /* look for specified options for the attributes
   * each option line looks something like:
   * "Options Type = Routine, Software Installation, Problem Fixed,
   * Configuration, Other"
   * Here "Options" is the indicator for an upcoming list of options for the
   * attribute named afterwards in the given example the attribute "Type" knows
   * the options "Routine", "Software Installation",... (you get it ;) )
   */

  QRegularExpression optionRegex(R"(Options (\w+))");
  QMap<QString, QStringList> options;
  for (const auto &k : keys) {
    auto match = optionRegex.match(k.c_str());
    if (!match.hasMatch()) {
      // the current ini key does not indicate an attribute option
      continue;
    }
    auto att = match.captured(1); // attribute to which the options apply

    auto attOpt = parseElogCfgLine(ini->Get(k, ""));
    options[att] = attOpt;
  }

  foreach (const auto &a, atts) {
    Attribute attribute;
    attribute.name = a;
    attribute.options =
        options[a]; // results in empty list when argument comes with no options
    attribute.required = req.contains(a) ? true : false;
    mAttributes << std::move(attribute);
    //    qDebug() << "added " << attribute.name << " with o " <<
    //    attribute.options
    //             << " req? " << attribute.required;
  }

  mElog.setElogProgramPath(ini->Get("elog_installation", "elog").c_str());
  mElog.setHost(ini->Get("elog_host", "localhost").c_str());
  mElog.setPort(ini->Get("elog_port", 8080));
  mElog.setLogbook(ini->Get("elog_logbook", "").c_str());
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
  return list;
}
