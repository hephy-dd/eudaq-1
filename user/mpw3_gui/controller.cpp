#include "controller.h"
#include "ui_controller.h"

#include <QRegExp>

Controller::Controller(QWidget *parent)
    : QWidget(parent), ui(new Ui::Controller), pearyDevName("RD50_MPW3") {
  ui->setupUi(this);
  QStringList logLevels;
  logLevels << "TRACE"
            << "DEBUG"
            << "INFO"
            << "WARNING"
            << "ERROR";
  ui->cbLogLevel->addItems(logLevels);
  ui->cbLogLevel->setCurrentIndex(1); // set default to DEBUG

  connect(&mSshProc, &QProcess::errorOccurred, this,
          &Controller::sshErrorOccured);
  connect(&mSshProc, &QProcess::readyReadStandardOutput, this,
          &Controller::receivedSshOutput);
  connect(&mSshProc, &QProcess::readyReadStandardError, this,
          &Controller::readSshError);
  connect(&mTimeOutTimer, &QTimer::timeout, this, &Controller::sshTimeout);

  mTimeOutTimer.setInterval(timeOutMs);
  mTimeOutTimer.setSingleShot(true);
}

Controller::~Controller() {
  sshCmdExecute("exit");
  mSshProc.terminate();
  delete ui;
}

void Controller::on_pbConnect_clicked() {
  if (mSshProc.isOpen() || mStateSsh == StateSsh::Running) {
    ui->tbLog->append("SSH connection already opened");
    return;
  }

  QStringList args;
  args << "-T" << ui->leSshConn->text();
  mSshProc.start("ssh", args);
  if (!mSshProc.isOpen()) {
    ui->tbLog->append("Error connecting to SSH server");
    return;
  }
  mSshProc.write(QString("echo " + connectedFeedback + "\n").toLocal8Bit());
}

void Controller::sshErrorOccured(QProcess::ProcessError error) {
  ui->tbLog->append("SSH error: " + QString::number(error));
}

void Controller::receivedSshOutput() {
  mSshWaitingForResponse = false; // for timeout to not triggered
  auto data = mSshProc.readAllStandardOutput();
  QString out(data);

  if (out.trimmed().startsWith(connectedFeedback)) {
    mStateSsh = StateSsh::Running;
  }
  switch (mStatePeary) {
  case StatePeary::NotInitialized:
    // start peary as soon as we connected to Caribou via SSH
    if (mStateSsh == StateSsh::Running) {
      startPeary();
    }
    break;
  case StatePeary::Initialized:
    // peary is started, waiting for user input
    // most likely commands are getting parsed now
    // don't do this automatically
    break;
  case StatePeary::GetCommands:
    if (populateCmds(out)) {
      mStatePeary = StatePeary::Operational;
      ui->cbCommands->setEnabled(true);
    }
    break;
  case StatePeary::Operational:
    break;
  }
  ui->tbLog->append(out);
}

void Controller::readSshError() {
  mSshWaitingForResponse = false; // error is also a response
  auto data = mSshProc.readAllStandardError();
  ui->tbLog->append("ERROR: " + QString(data));
}

void Controller::sshTimeout() {
  if (mSshWaitingForResponse) {
    ui->tbLog->append("ERROR: SSH command timeout");
  }
}

void Controller::startPeary() {
  QString cmd = "pearycli -v " + ui->cbLogLevel->currentText() + " -c " +
                ui->leConfigFile->text() + " " + pearyDevName;
  sshCmdExecute(cmd);
  mStatePeary = StatePeary::Initialized;
  ui->pbInitCmds->setEnabled(
      true); // TODO: check if stuff did work before enabling further options
  ui->pbPower->setEnabled(true);
}

bool Controller::populateCmds(const QString &pearyOutput) {
  ui->cbCommands->clear();
  auto commands = pearyOutput.split("\n");
  if (commands.length() <= 5) {
    // there should be at least 5 commands available, otherwise this most likely
    // is not the input wanted peary-output
    return false;
  }
  for (int i = 1; i < commands.length(); i++) {
    // firstentry is blabla
    QString command = commands[i].trimmed(); // remove whitespace
    if (command.length() >= 3) // empty line or "waiting for input" line
      ui->cbCommands->addItem(command);
  }

  return true;
}

void Controller::sshCmdExecute(const QString &command, bool cmdForPeary,
                               int devId) {
  if (mStateSsh != StateSsh::Running) {
    ui->tbLog->append("ERROR: Cannot execute command! SSH not connected");
    return;
  }

  QString cmd = command;
  if (cmdForPeary) {
    // a command for peary needs to contain the device ID as last argument
    cmd += " " + QString::number(devId);
  }
  cmd += "\n"; // basically pressing enter (for ssh to actually execute command)
  mSshProc.write(cmd.toLocal8Bit());

  mSshWaitingForResponse = true;
  mTimeOutTimer.start();
}

void Controller::on_pbClrLog_clicked() { ui->tbLog->clear(); }

void Controller::on_pbTest_clicked() {
  mSshWaitingForResponse = true;
  mTimeOutTimer.start();
}

void Controller::on_pbInitCmds_clicked() {

  if (mStatePeary == StatePeary::NotInitialized ||
      mStateSsh != StateSsh::Running) {
    ui->tbLog->append("ERROR: not ready to load commands");
    return;
  }
  sshCmdExecute("listCommands", true);
  mStatePeary = StatePeary::GetCommands;
}

void Controller::on_pbExecute_clicked() {
  QString cmd = ui->cbCommands->currentText();
  if (cmd.isEmpty()) {
    ui->tbLog->append(
        "Kind suggestion: choose a command before trying to execute it ;)");
    return;
  }
  QRegExp regExp("\\(.+"); // we want to remove the (args: -- ) part
  // this regular expression matches everything starting from "("
  cmd = cmd.remove(regExp) + " " + ui->leArgs->text();
  sshCmdExecute(cmd, true);
}

void Controller::on_pbConfigure_clicked() {
  sshCmdExecute("configure", true);
  ui->pbExecute->setEnabled(true);
}

void Controller::on_pbPower_toggled(bool checked) {
  if (checked) {
    sshCmdExecute("powerOn", true);
    ui->pbConfigure->setEnabled(true); // TODO: check if powerOn did work

  } else {
    sshCmdExecute("powerOff", true);
    ui->pbConfigure->setEnabled(false);
    ui->pbExecute->setEnabled(false);
  }
}

void Controller::on_pbReset_clicked() {
  mSshProc.close();
  mStatePeary = StatePeary::NotInitialized;
  mStateSsh = StateSsh::NotInitialized;
  ui->pbConfigure->setEnabled(false);
  ui->pbPower->setEnabled(false);
  ui->pbPower->setChecked(false);
  ui->pbInitCmds->setEnabled(false);
  ui->pbExecute->setEnabled(false);
  ui->cbCommands->clear();
  ui->cbCommands->setEnabled(false);

  ui->tbLog->append("\n\n\n ....RESET.... \n\n\n");
}
