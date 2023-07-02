#include "eloggui.h"
#include "ui_eloggui.h"

#include <QApplication>

#include <QColor>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTime>

ElogGui::ElogGui(const std::string name, const std::string &runcontrol,
                 QWidget *parent)
    : QWidget(parent), ui(new Ui::ElogGui),
      mSettings(QSettings("EUDAQ collaboration", "EUDAQ")),
      mProxy(name, runcontrol, this) {
  ui->setupUi(this);
  this->show();
  connect(ui->pbSubmit, &QPushButton::clicked, this, &ElogGui::submit);
  connect(ui->lePass, &QLineEdit::textChanged, &mElog, &Elog::setPass);
  connect(ui->leUser, &QLineEdit::textChanged, &mElog, &Elog::setUser);

  qRegisterMetaType<eudaq::ConfigurationSPC>(
      "eudaq::ConfigurationSPC"); // needed for signal and slots machinery

  // connect our slots to the EUDAQ intewrface
  connect(&mProxy, &Producer2GUIProxy::initialize, this,
          &ElogGui::DoInitialise);
  connect(&mProxy, &Producer2GUIProxy::configure, this, &ElogGui::DoConfigure);
  connect(&mProxy, &Producer2GUIProxy::reset, this, &ElogGui::DoReset);
  connect(&mProxy, &Producer2GUIProxy::start, this, &ElogGui::DoStartRun);
  connect(&mProxy, &Producer2GUIProxy::stop, this, &ElogGui::DoStopRun);
  connect(&mProxy, &Producer2GUIProxy::terminate, this, &ElogGui::DoTerminate);

  mSettings.beginGroup("ElogProducer:" + QString(name.c_str()));

  setWindowTitle(name.c_str());

  ui->twAtt->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

ElogGui::~ElogGui() { delete ui; }

std::string ElogGui::Connect() { return mProxy.Connect(); }

void ElogGui::DoInitialise(const eudaq::ConfigurationSPC &ini) {
  elogSetup(ini);
  populateUi();

  auto color = palette().color(QPalette::Window);
  QString colorStr = ini->Get<std::string>("color", "").c_str();
  if (!colorStr.isEmpty()) {
    color = QColor(colorStr);
  }

  QPalette pal;
  pal.setColor(QPalette::Window, color);
  setAutoFillBackground(true);
  setPalette(pal);

  auto pass = mSettings.value("pass").toString();
  auto user = mSettings.value("user").toString();

  ui->leUser->setText(user);
  ui->lePass->setText(pass);

  mElog.setPass(pass);
  mElog.setUser(user);
}

void ElogGui::DoConfigure(const eudaq::ConfigurationSPC &conf) {
  mStartCmd = conf->Get("start_cmd", "").c_str();
  mFiles2Log = QString(conf->Get("files2log", "").c_str()).split(',');
}

void ElogGui::DoStartRun(int runNmb) {
  mStartTime = QDateTime::currentDateTime();
  mCurrRunN = runNmb;

  qDebug() << mStartCmd;
  /*
   * execute an additional command (eg a shell script) before starting the run
   * useful to copy some remote configurationfiles from different computers to
   * the local one for attaching them to the Elog entry
   */
  if (!mStartCmd.isEmpty()) {
    QProcess proc;
    proc.setProgram(mStartCmd);
    proc.start();
    if (!proc.waitForFinished()) {
      qWarning() << "start_cmd " << mStartCmd << " timed out / failed "
                 << proc.exitCode();
    }
    qDebug() << "returned " << proc.exitCode() << " " << proc.exitStatus();
  }
}

void ElogGui::DoStopRun() {
  mStopTime = QDateTime::currentDateTime();
  if (ui->cbAutoSubmit->isChecked()) {
    submit(true);
  }
}

void ElogGui::DoReset() { mElog.reset(); }

void ElogGui::DoTerminate() {
  saveCurrentElogSetup();
  close();
}

bool ElogGui::submit(bool autoSubmit) {
  auto msg = ui->teMessage->toPlainText();
  if (autoSubmit) {
    msg = QString("automatic log for run %1").arg(mCurrRunN);
    msg += "\nComment:\n" + ui->teMessage->toPlainText();
  }
  auto succ = mElog.submitEntry(attributesSet(), msg, autoSubmit, mCurrRunN,
                                mStartTime, mStopTime, mFiles2Log);
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

    saveCurrentElogSetup();

  } else {
    ui->tbLog->insertPlainText(
        QString("Error sending log: %1\n")
            .arg(QTime::currentTime().toString("hh:mm:ss")));
  }

  return succ;
}

void ElogGui::populateUi() {
  int i = 0;
  foreach (const auto &att, mAttributes) {

    auto item = new QTableWidgetItem(att.name);
    ui->twAtt->setRowCount(i + 1);
    ui->twAtt->setItem(i, 0, item);
    if (!att.options.isEmpty()) {
      auto cb = new NonScrollingCombobox;
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
      if (item != nullptr) {
        item->setData(Qt::DisplayRole, mSettings.value(att.name));
        i++;
        continue;
      } else if (widget == nullptr) {
        i++;
        continue;
      }
      auto cb = qobject_cast<NonScrollingCombobox *>(widget);
      if (cb == nullptr) {
        i++;
        continue;
      }
      cb->setCurrentText(mSettings.value(att.name).toString());
    }
    i++;
  }
}

void ElogGui::elogSetup(const eudaq::ConfigurationSPC &ini) {
  auto attributes = ini->Get("Attributes", "");
  auto reqAtt = ini->Get("Required Attributes", "");

  ui->twAtt->setRowCount(0);
  mAttributes.clear();

  auto atts = parseElogCfgLine(attributes);
  auto req = parseElogCfgLine(reqAtt);

  auto keys = ini->Keylist();

  auto startTAtt = ini->Get("att_start_time", "Start-Time");
  auto stopTAtt = ini->Get("att_stop_time", "Stop-Time");
  auto runNmbAtt = ini->Get("att_run_number", "Run-ID");
  auto runDurAtt = ini->Get("att_run_duration", "");
  mElog.setAttStopT(stopTAtt.c_str());
  mElog.setAttStartT(startTAtt.c_str());
  mElog.setAttRunNmb(runNmbAtt.c_str());
  mElog.setAttRunDur(runDurAtt.c_str());
  QStringList specialAtt;
  specialAtt << mElog.attRunNmb() << mElog.attStartT() << mElog.attStopT()
             << mElog.attRunDur();

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
  auto host = QString(ini->Get("elog_host", "localhost").c_str());
  mElog.setHost(host);
  mElog.setPort(ini->Get("elog_port", 8080));
  auto logbook = QString(ini->Get("elog_logbook", "").c_str());
  mElog.setLogbook(logbook);

  ui->lLogbook->setText(host + " : " + logbook);
}

QStringList ElogGui::parseElogCfgLine(const std::string &key) {
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

QList<ElogGui::SetAtt> ElogGui::attributesSet() {
  QList<ElogGui::SetAtt> attributes;
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
    auto cb = qobject_cast<NonScrollingCombobox *>(w);
    if (cb != nullptr) {
      att.second = cb->currentText();
    }
    attributes << att;
  }
  return attributes;
}

bool ElogGui::saveCurrentElogSetup() {
  mSettings.setValue("pass", ui->lePass->text());
  mSettings.setValue("user", ui->leUser->text());

  auto setAtts = attributesSet();
  foreach (const auto &att, setAtts) {
    mSettings.setValue(att.first, att.second);
  }

  return true;
}
