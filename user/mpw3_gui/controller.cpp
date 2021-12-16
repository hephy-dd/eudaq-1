#include "controller.h"
#include "ui_controller.h"

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

  connect(&mSshProc, &QProcess::errorOccurred, this,
          &Controller::sshErrorOccured);
  connect(&mSshProc, &QProcess::readyReadStandardOutput, this,
          &Controller::sshRead);
  connect(&mSshProc, &QProcess::readyReadStandardError, this,
          &Controller::sshError);
}

Controller::~Controller() {
  delete ui;
  mSshProc.terminate();
}

void Controller::on_pbConnect_clicked() {
  if (mSshProc.isOpen()) {
    ui->tbLog->append("SSH connection already open");
    return;
  }
  QStringList args;
  args << ui->leSshConn->text();
  mSshProc.start("ssh", args);
  if (!mSshProc.isOpen()) {
    ui->tbLog->append("Error connecting to SSH server");
  }
  mSshProc.write(QString("echo Connected\n").toLocal8Bit()); // test connection
}

void Controller::sshErrorOccured(QProcess::ProcessError error) {
  ui->tbLog->append("SSH error: " + QString::number(error));
}

void Controller::sshRead() {
  auto data = mSshProc.readAllStandardOutput();
  ui->tbLog->append("read: " + QString(data));
}

void Controller::sshError() {
  auto data = mSshProc.readAllStandardError();
  ui->tbLog->append("ERROR: " + QString(data));
}

void Controller::startPeary() {
  QString cmd = "pearycli -v " + ui->cbLogLevel->currentText() + " -c " +
                ui->leConfigFile->text() + " RD50_MPW3\n";
  mSshProc.write(cmd.toLocal8Bit());
}

QByteArray Controller::pearyCmd(const QString &cmd, int devId) {
  return QString(cmd + " " + QString::number(devId) + "\n").toLocal8Bit();
}

void Controller::on_pbClrLog_clicked() { ui->tbLog->clear(); }

void Controller::on_pbTest_clicked() {

  mSshProc.write(QString("echo test\n").toLocal8Bit());
}

void Controller::on_pbInitCmds_clicked() {
  startPeary();
  mSshProc.write(pearyCmd("listCommands"));
}
