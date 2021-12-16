#include "controller.h"
#include "ui_controller.h"

#include <QRegExp>

Controller::Controller(QWidget *parent)
    : QWidget(parent), ui(new Ui::Controller) {
  ui->setupUi(this);
  QStringList logLevels;
  logLevels << "TRACE"
            << "DEBUG"
            << "INFO"
            << "WARNING"
            << "ERROR";
  ui->cbLogLevel->addItems(logLevels);
  ui->cbLogLevel->setCurrentIndex(1);

  connect(&mSshProc, &QProcess::errorOccurred, this,
          &Controller::sshErrorOccured);
  connect(&mSshProc, &QProcess::readyReadStandardOutput, this,
          &Controller::readSshStdOut);
  connect(&mSshProc, &QProcess::readyReadStandardError, this,
          &Controller::readSshError);
}

Controller::~Controller() {
  sshCmdExecute("exit");
  mSshProc.terminate();
  delete ui;
}

void Controller::on_pbConnect_clicked() {
  if (mSshProc.isOpen() && mStateSsh == StateSsh::Running) {
    ui->tbLog->append("SSH connection already open");
    return;
  }

  QStringList args;
  args << ui->leSshConn->text();
  mSshProc.start("ssh", args);
  if (!mSshProc.isOpen()) {
    ui->tbLog->append("Error connecting to SSH server");
    return;
  }
  mSshProc.write(QString("echo " + connectedFeedback + "\n")
                     .toLocal8Bit()); // test connection
}

void Controller::sshErrorOccured(QProcess::ProcessError error) {
  ui->tbLog->append("SSH error: " + QString::number(error));
}

void Controller::readSshStdOut() {
  auto data = mSshProc.readAllStandardOutput();
  QString out(data);

  if (out.trimmed().startsWith(connectedFeedback)) {
    mStateSsh = StateSsh::Running;
  }
  switch (mStatePeary) {
  case StatePeary::NotInitialized:
    if (mStateSsh == StateSsh::Running) {
      startPeary();
    }
    break;
  case StatePeary::Initialized:
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
  auto data = mSshProc.readAllStandardError();
  ui->tbLog->append("ERROR: " + QString(data));
}

void Controller::startPeary() {
  QString cmd = "pearycli -v " + ui->cbLogLevel->currentText() + " -c " +
                ui->leConfigFile->text() + " " + pearyDevice;
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

void Controller::sshCmdExecute(const QString &command, bool pearyCmd,
                               int devId) {
  if (mStateSsh != StateSsh::Running) {
    ui->tbLog->append("ERROR: Cannot execute command! SSH not connected");
    return;
  }

  QString cmd = command;
  if (pearyCmd) {
    // a command for peary needs to contain the device ID as last argument
    cmd += " " + QString::number(devId);
  }
  cmd += "\n"; // basically pressing enter for ssh to actually execute command
  mSshProc.write(cmd.toLocal8Bit());
}

void Controller::on_pbClrLog_clicked() { ui->tbLog->clear(); }

void Controller::on_pbTest_clicked() { startPeary(); }

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
