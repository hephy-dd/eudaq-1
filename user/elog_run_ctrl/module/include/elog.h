#ifndef ELOG_H
#define ELOG_H

#include <QObject>
#include <QProcess>

class Elog : public QObject
{
    Q_OBJECT
public:
    explicit Elog(const QString &elogPath, QObject *parent = nullptr);
    ~Elog();

    const QString &host() const;
    void setHost(const QString &newHost);

    const QString &logbook() const;
    void setLogbook(const QString &newLogbook);

private:
    QProcess mElog;
    QString mHost, mLogbook, mUser, mPass;


};

#endif // ELOG_H
