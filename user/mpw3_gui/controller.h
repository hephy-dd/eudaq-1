#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QProcess>
#include <QWidget>

namespace Ui {
  class Controller;
}

class Controller : public QWidget {
  Q_OBJECT

public:
  explicit Controller(QWidget *parent = nullptr);
  ~Controller();

private slots:
  void on_pbPower_toggled(bool checked);

private slots:
  void on_pbConfigure_clicked();

private slots:
  void on_pbExecute_clicked();

private slots:
  void on_pbInitCmds_clicked();

private slots:
  void on_pbTest_clicked();

private slots:
  void on_pbClrLog_clicked();

private slots:
  void on_pbConnect_clicked();

  void sshErrorOccured(QProcess::ProcessError error);
  void readSshStdOut();
  void readSshError();

private:
  enum class StateSsh { NotInitialized, Running, Error };
  enum class StatePeary {
    NotInitialized,
    Initialized,
    GetCommands,
    Operational
  };

  void startPeary();
  bool populateCmds(const QString &pearyOutput);
  void sshCmdExecute(const QString &command, bool pearyCmd = false,
                     int devId = 0);

  const QString connectedFeedback = "[Controller CONNECTED]";
  const QString pearyDevice = "RD50_MPW3";

  Ui::Controller *ui;
  QProcess mSshProc;
  StateSsh mStateSsh = StateSsh::NotInitialized;
  StatePeary mStatePeary = StatePeary::NotInitialized;
};

#endif // CONTROLLER_H
