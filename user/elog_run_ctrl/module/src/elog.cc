#include "elog.h"

#include <QDebug>
#include <QFile>

Elog::Elog(QObject *parent) : QObject{parent} {}

Elog::~Elog() {}

const QString &Elog::host() const { return mHost; }

void Elog::setHost(const QString &newHost) { mHost = newHost; }

const QString &Elog::logbook() const { return mLogbook; }

void Elog::setLogbook(const QString &newLogbook) { mLogbook = newLogbook; }

void Elog::setElogProgramPath(const QString &path) { mProc.setProgram(path); }

uint32_t Elog::port() const { return mPort; }

void Elog::setPort(uint32_t newPort) { mPort = newPort; }

bool Elog::submitEntry(const QList<QPair<QString, QString>> &attributes,
                       const QString &message) {
  QFile msgFile(tmpFile);
  if (!msgFile.open(QIODevice::WriteOnly)) {
    qWarning("error opening temp file");
    return false;
  }
  // write message to temporary file to forward to elog program
  QTextStream ts(&msgFile);
  ts << message;
  ts.flush();

  QStringList args;
  args << "-h" << mHost;
  args << "-l" << mLogbook;
  args << "-p" << QString::number(mPort);
  args << "-m" << tmpFile;
  foreach (const auto &att, attributes) {
    /*
     * attribues have to be given by
     * -a <Name>=<value>
     * eg
     * -a Type=Software
     */
    args << "-a" << QString("%1=%2").arg(att.first, att.second);
  }
  mProc.setArguments(args);
  mProc.start();
  auto finished = mProc.waitForFinished(5000);
  auto exitCode = mProc.exitCode();
  if (!finished) {
    qWarning() << "timeout sending elog message";
    qDebug() << mProc.readAllStandardError() << mProc.readAllStandardOutput();
    return false;
  }
  if (exitCode != 0) {
    qWarning() << "error submitting elog: " << mProc.readAllStandardOutput();
    return false;
  }
  auto output = mProc.readAllStandardOutput();
  /*
   * elog program does not return !=0 in all error cases
   * parse and process output instead
   */
  if (output.contains("Error")) {
    qWarning() << output;
    return false;
  }
  if (output.contains("successfully")) {
    return true;
  }
  qWarning() << output;
  return false;
}

bool Elog::reset() { return mProc.reset(); }

void Elog::debugPrint() {
  qDebug() << mPort << mLogbook << mHost << mProc.program();
}
