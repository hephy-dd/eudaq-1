#ifndef ELOG_H
#define ELOG_H

#include <QObject>
#include <QProcess>

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
  uint32_t port() const;
  void setPort(uint32_t newPort);
  void setUser(const QString &newUser);
  void setPass(const QString &newPass);

  bool submitEntry(const QList<QPair<QString, QString>> &attributes,
                   const QString &message, bool autoSubmit = false,
                   int runNmb = -1, int nEvents = 0, const QString &startTime = QString(),
                   const QString &stopTime = QString(), const QString &configFile = QString());

  bool reset();
  void debugPrint();

  const QString &attStartT() const;
  void setAttStartT(const QString &newAttStartT);

  const QString &attStopT() const;
  void setAttStopT(const QString &newAttStopT);

  const QString &attRunNmb() const;
  void setAttRunNmb(const QString &newAttRunNmb);

  QString attEventCnt() const;
  void setAttEventCnt(const QString &newAttEventCnt);

private:
  const QString tmpFile = "/tmp/elog_msg";
  QProcess mProc;
  QString mHost, mLogbook, mUser, mPass, mAttStartT, mAttStopT, mAttRunNmb, mAttEventCnt;
  uint32_t mPort;
};

#endif // ELOG_H
