#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTime>

namespace {
auto dummy0 = eudaq::Factory<eudaq::RunControl>::Register<ElogRunCtrl,
                                                          const std::string &>(
    ElogRunCtrl::m_id_factory);
}

ElogRunCtrl::ElogRunCtrl(const std::string &listenaddress, QWidget *parent)
    : QWidget(parent), eudaq::RunControl(listenaddress),
      ui(new Ui::ElogRunCtrl),
      mSettings(QSettings("EUDAQ collaboration", "EUDAQ")) {
  ui->setupUi(this);
  this->show();
  connect(ui->pbSubmit, &QPushButton::clicked, this, &ElogRunCtrl::submit);
  mSettings.beginGroup("ElogRC");
}

ElogRunCtrl::~ElogRunCtrl() { delete ui; }

void ElogRunCtrl::Initialise() {
  RunControl::Initialise();

  elogSetup();
  populateUi();
}

void ElogRunCtrl::Configure() {
  RunControl::Configure();
  // store path to config file for later attaching it to log
  mConfigFile = GetConfiguration()->Name().c_str();
}

void ElogRunCtrl::StartRun() {
  RunControl::StartRun();
  mStartTime = QDateTime::currentDateTime();
  mCurrRunN = GetRunN();
}

void ElogRunCtrl::StopRun() {
  RunControl::StopRun();
  mStopTime = QDateTime::currentDateTime();
  if (ui->cbAutoSubmit->isChecked()) {
    submit(true);
  }
}

void ElogRunCtrl::Exec() { RunControl::Exec(); }

void ElogRunCtrl::Reset() {
  RunControl::Reset();
  mElog.reset();
}

void ElogRunCtrl::Terminate() {
  RunControl::Terminate();
  saveCurrentElogSetup();
  close();
}

void ElogRunCtrl::submit(bool autoSubmit) {
  auto msg = ui->teMessage->toPlainText();
  if (autoSubmit) {
    msg = QString("automatic log for run %1").arg(mCurrRunN);
    msg += "\nComment:\n" + ui->teMessage->toPlainText();
  }
  auto succ = mElog.submitEntry(
      attributesSet(), msg, autoSubmit, mCurrRunN, eventsCurrRun(),
      mStartTime.toString("dd.MM.yyyy hh:mm:ss"),
      mStopTime.toString("dd.MM.yyyy hh:mm:ss"), mConfigFile);
  if (succ) {
    if (autoSubmit) {
      ui->tbLog->insertPlainText(
          QString("Sent auto log: %1\n")
              .arg(QTime::currentTime().toString("hh:mm:ss")));
    } else {
      ui->tbLog->insertPlainText(
          QString("Sent manual log: %1\n")
              .arg(QTime::currentTime().toString("hh:mm:ss")));
    }
  } else {
    ui->tbLog->insertPlainText(
        QString("Error sending log: %1\n")
            .arg(QTime::currentTime().toString("hh:mm:ss")));
  }
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
    } else {
      ui->twAtt->setItem(i, 1, new QTableWidgetItem());
    }

    // restore old settings if there are some in cache matching our config
    if (mSettings.allKeys().contains(att.name)) {
      auto item = ui->twAtt->item(i, 1);
      auto widget = ui->twAtt->cellWidget(i, 1);
      i++;
      if (item != nullptr) {
        item->setData(Qt::DisplayRole, mSettings.value(att.name));
        continue;
      } else if (widget == nullptr) {
        continue;
      }
      auto cb = qobject_cast<QComboBox *>(widget);
      if (cb == nullptr) {
        continue;
      }
      cb->setCurrentText(mSettings.value(att.name).toString());
    }
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

  mEventCntConn = ini->Get("event_cnt_conn", "").c_str();

  auto startTAtt = ini->Get("att_start_time", "Start-Time");
  auto stopTAtt = ini->Get("att_stop_time", "Stop-Time");
  auto runNmbAtt = ini->Get("att_run_number", "Run-ID");
  auto nEventAtt = ini->Get("att_event_cnt", "");
  mElog.setAttStopT(stopTAtt.c_str());
  mElog.setAttStartT(startTAtt.c_str());
  mElog.setAttRunNmb(runNmbAtt.c_str());
  mElog.setAttEventCnt(nEventAtt.c_str());
  QStringList specialAtt;
  specialAtt << mElog.attRunNmb() << mElog.attStartT() << mElog.attStopT();

  /* look for specified options for the attributes
   * each option line looks something like:
   * "Options Type = Routine, Software Installation, Problem Fixed,
   * Configuration, Other"
   * Here "Options" is the indicator for an upcoming list of options for the
   * attribute named afterwards in the given example the attribute "Type" knows
   * the options "Routine", "Software Installation",... (you get it ;) )
   */

  QRegularExpression optionRegex(R"(Options (.+))");
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
    if (specialAtt.contains(a)) {
      // skip special attributes used for auto Elog submission
      continue;
    }
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
  mElog.setUser(ini->Get("user", "").c_str());
  mElog.setPass(ini->Get("password", "").c_str());
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

int ElogRunCtrl::eventsCurrRun() {
  auto conns = GetActiveConnectionStatusMap();
  for (const auto &conn : conns) {
    qDebug() << conn.first->GetName().c_str();
    // look for the connection from which we should extract number of events
    if (mEventCntConn != conn.first->GetName().c_str()) {
      continue;
    }
    auto tags = conn.second->GetTags();
    for (const auto &tag : tags) {
      // tag EventN specifies the number of events during the current run
      if (QString(tag.first.c_str()).trimmed() != "EventN") {
        continue;
      }
      bool ok;
      QString tmp(tag.second.c_str());
      auto eventCnt = tmp.toInt(&ok);
      if (ok) {
        return eventCnt;
      }
    }
  }
  return -1;
}

QList<ElogRunCtrl::SetAtt> ElogRunCtrl::attributesSet() {
  QList<ElogRunCtrl::SetAtt> attributes;
  for (int i = 0; i < ui->twAtt->rowCount(); i++) {
    SetAtt att;
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
  return attributes;
}

bool ElogRunCtrl::saveCurrentElogSetup() {
  auto setAtts = attributesSet();
  foreach (const auto &att, setAtts) {
    mSettings.setValue(att.first, att.second);
  }

  return true;
}
