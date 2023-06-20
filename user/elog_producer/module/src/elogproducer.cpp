#include "elogproducer.h"
#include "ui_elogproducer.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTime>

namespace {
auto dummy0 =
    eudaq::Factory<eudaq::Producer>::Register<ElogProducer, const std::string &,
                                              const std::string &>(
        ElogProducer::m_id_factory);
}

ElogProducer::ElogProducer(const std::string name,
                           const std::string &runcontrol, QWidget *parent)
    : QWidget(parent), eudaq::Producer(name, runcontrol),
      ui(new Ui::ElogProducer),
      mSettings(QSettings("EUDAQ collaboration", "EUDAQ")) {
  ui->setupUi(this);
  this->show();
  connect(ui->pbSubmit, &QPushButton::clicked, this, &ElogProducer::submit);
  connect(ui->lePass, &QLineEdit::textChanged, &mElog, &Elog::setPass);
  connect(ui->leUser, &QLineEdit::textChanged, &mElog, &Elog::setUser);
  mSettings.beginGroup("ElogProducer");
}

ElogProducer::~ElogProducer() { delete ui; }

void ElogProducer::DoInitialise() {
  elogSetup();
  populateUi();
}

void ElogProducer::DoConfigure() {
  // store path to config file for later attaching it to log
  mConfigFile = GetConfiguration()->Name().c_str();
}

void ElogProducer::DoStartRun() {
  mStartTime = QDateTime::currentDateTime();
  mCurrRunN = GetRunNumber();
}

void ElogProducer::DoStopRun() {
  mStopTime = QDateTime::currentDateTime();
  if (ui->cbAutoSubmit->isChecked()) {
    submit(true);
  }
}

void ElogProducer::DoReset() { mElog.reset(); }

void ElogProducer::DoTerminate() {
  saveCurrentElogSetup();
  close();
}

void ElogProducer::submit(bool autoSubmit) {
  auto msg = ui->teMessage->toPlainText();
  if (autoSubmit) {
    msg = QString("automatic log for run %1").arg(mCurrRunN);
    msg += "\nComment:\n" + ui->teMessage->toPlainText();
  }
  auto succ = mElog.submitEntry(attributesSet(), msg, autoSubmit, mCurrRunN,
                                mStartTime, mStopTime, mConfigFile);
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

void ElogProducer::populateUi() {
  qDebug() << "1";
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
    qDebug() << "2";
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

    auto pass = mSettings.value("pass").toString();
    auto user = mSettings.value("user").toString();

    mElog.setPass(pass);
    mElog.setUser(user);
    qDebug() << "3";
  }
}

void ElogProducer::elogSetup() {
  auto ini = GetInitConfiguration();
  auto attributes = ini->Get("Attributes", "");
  auto reqAtt = ini->Get("Required Attributes", "");
  qDebug() << "1";

  ui->twAtt->setRowCount(0);
  mAttributes.clear();

  auto atts = parseElogCfgLine(attributes);
  auto req = parseElogCfgLine(reqAtt);

  auto keys = ini->Keylist();

  mEventCntConn = ini->Get("event_cnt_conn", "").c_str();

  auto startTAtt = ini->Get("att_start_time", "Start-Time");
  auto stopTAtt = ini->Get("att_stop_time", "Stop-Time");
  auto runNmbAtt = ini->Get("att_run_number", "Run-ID");
  auto runDurAtt = ini->Get("att_run_duration", "");
  mElog.setAttStopT(stopTAtt.c_str());
  mElog.setAttStartT(startTAtt.c_str());
  mElog.setAttRunNmb(runNmbAtt.c_str());
  mElog.setAttRunDur(runDurAtt.c_str());
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

  qDebug() << "2";
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
  qDebug() << "3";
  mElog.setElogProgramPath(ini->Get("elog_installation", "elog").c_str());
  mElog.setHost(ini->Get("elog_host", "localhost").c_str());
  mElog.setPort(ini->Get("elog_port", 8080));
  mElog.setLogbook(ini->Get("elog_logbook", "").c_str());
}

QStringList ElogProducer::parseElogCfgLine(const std::string &key) {
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

QList<ElogProducer::SetAtt> ElogProducer::attributesSet() {
  QList<ElogProducer::SetAtt> attributes;
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

bool ElogProducer::saveCurrentElogSetup() {
  mSettings.setValue("pass", ui->lePass->text());
  mSettings.setValue("user", ui->leUser->text());

  auto setAtts = attributesSet();
  foreach (const auto &att, setAtts) {
    mSettings.setValue(att.first, att.second);
  }

  return true;
}
