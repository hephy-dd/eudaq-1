#ifndef ELOG_H
#define ELOG_H

#include <QObject>
#include <QProcess>
#include <QDateTime>

class Elog : public QObject {
  Q_OBJECT
public:
  explicit Elog(QObject *parent = nullptr);
  ~Elog();

  const QString &host() const;
  void setHost(const QString &newHost);
  const QString &logbook() const;
  void setLogbook(const QString &newLogbook);
  void setElogProgramPath(const QString &path);

  bool submitEntry(const QList<QPair<QString, QString>> &attributes,
                   const QString &message, bool autoSubmit = false,
                   int runNmb = -1, const QDateTime &startTime = QDateTime(),
                   const QDateTime &stopTime = QDateTime(), const QString &configFile = QString());

  bool reset();
  void debugPrint();
public slots:
  uint32_t port() const;
  void setPort(uint32_t newPort);

  void setUser(const QString &newUser);
  void setPass(const QString &newPass);

  const QString &attStartT() const;
  void setAttStartT(const QString &newAttStartT);

  const QString &attStopT() const;
  void setAttStopT(const QString &newAttStopT);

  const QString &attRunNmb() const;
  void setAttRunNmb(const QString &newAttRunNmb);

  QString attRunDur() const;
  void setAttRunDur(const QString &newAttRunDur);

private:
  const QString tmpFile = "/tmp/elog_msg";
  QProcess mProc;
  QString mHost, mLogbook, mUser, mPass, mAttStartT, mAttStopT, mAttRunNmb, mAttRunDur;
  uint32_t mPort;
};

#endif // ELOG_H
