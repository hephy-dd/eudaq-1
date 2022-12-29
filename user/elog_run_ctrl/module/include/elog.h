#ifndef ELOG_H
#define ELOG_H

#include <QObject>
#include <QProcess>

class Elog : public QObject
{
    Q_OBJECT
public:
    explicit Elog(QObject *parent = nullptr);
    ~Elog();

    const QString &host() const;
    void setHost(const QString &newHost);
    const QString &logbook() const;
    void setLogbook(const QString &newLogbook);
    void setElogProgramPath(const QString &path);
    uint32_t port() const;
    void setPort(uint32_t newPort);

    bool reset();
    void debugPrint();

private:
    QProcess mProc;
    QString mHost, mLogbook, mUser, mPass;
    uint32_t mPort;


};

#endif // ELOG_H
