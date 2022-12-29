#include "elog.h"

#include <QDebug>

Elog::Elog(QObject *parent) : QObject{parent} {}

Elog::~Elog() {}

const QString &Elog::host() const { return mHost; }

void Elog::setHost(const QString &newHost) { mHost = newHost; }

const QString &Elog::logbook() const { return mLogbook; }

void Elog::setLogbook(const QString &newLogbook) { mLogbook = newLogbook; }

void Elog::setElogProgramPath(const QString &path) { mProc.setProgram(path); }

uint32_t Elog::port() const { return mPort; }

void Elog::setPort(uint32_t newPort) { mPort = newPort; }

bool Elog::reset() { return mProc.reset(); }

void Elog::debugPrint() {
  qDebug() << mPort << mLogbook << mHost << mProc.program();
}
