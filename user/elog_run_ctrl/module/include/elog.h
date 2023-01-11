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

  bool submitEntry(const QList<QPair<QString, QString>> &attributes,
                   const QString &message, bool autoSubmit = false,
                   int runNmb = -1, const QString &startTime = QString(),
                   const QString &stopTime = QString());

  bool reset();
  void debugPrint();

  const QString &attStartT() const;
  void setAttStartT(const QString &newAttStartT);

  const QString &attStopT() const;
  void setAttStopT(const QString &newAttStopT);

  const QString &attRunNmb() const;
  void setAttRunNmb(const QString &newAttRunNmb);

private:
  const QString tmpFile = "/tmp/elog_msg";
  QProcess mProc;
  QString mHost, mLogbook, mUser, mPass, mAttStartT, mAttStopT, mAttRunNmb;
  uint32_t mPort;
};

#endif // ELOG_H
