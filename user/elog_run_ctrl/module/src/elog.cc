#include "elog.h"

Elog::Elog(const QString &elogPath, QObject *parent) : QObject{parent} {
    mElog.setProgram(elogPath);
}

Elog::~Elog()
{

}

const QString &Elog::host() const { return mHost; }

void Elog::setHost(const QString &newHost) { mHost = newHost; }

const QString &Elog::logbook() const { return mLogbook; }

void Elog::setLogbook(const QString &newLogbook) { mLogbook = newLogbook; }
