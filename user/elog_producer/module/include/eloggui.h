#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include "elog.h"
#include "producer2guiproxy.h"

#include <QComboBox>
#include <QDateTime>
#include <QMainWindow>
#include <QMap>
#include <QSettings>
#include <QWheelEvent>
#include <QWidget>

namespace Ui {
class ElogGui;
}

Q_DECLARE_METATYPE(
    eudaq::ConfigurationSPC) // neded for signal and slots machinery

/**
 * @brief The NonScrollingCombobox class
 * only here to disable the scrolling of the combobox items when a mousewheel is
 * scrolling on it this can be annoying when scrolliing through the table and
 * one accidentially changes values thereby
 */
class NonScrollingCombobox : public QComboBox {

  Q_OBJECT

public:
  NonScrollingCombobox(QWidget *parent = 0) : QComboBox(parent) {
    setFocusPolicy(Qt::StrongFocus);
  }

protected:
  virtual void wheelEvent(QWheelEvent *event) {
    if (!hasFocus()) {
      event->ignore();
    } else {
      /*
       * when we have strong focus, aka user opened the combobox by clicking on
       * it earlier, the scrolling is enabled, otherwise ignored
       */
      QComboBox::wheelEvent(event);
    }
  }
};

class ElogGui : public QWidget {
  Q_OBJECT

public:
  explicit ElogGui(const std::string name, const std::string &runcontrol = "",
                   QWidget *parent = nullptr);
  ~ElogGui();

  static const uint32_t m_id_factory = eudaq::cstr2hash("elog");
  std::string Connect();

private slots:
  void submit(bool autoSubmit = false);
  void DoInitialise(const eudaq::ConfigurationSPC &ini);
  void DoConfigure(const eudaq::ConfigurationSPC &conf);
  void DoStartRun(int runNmb);
  void DoStopRun();
  void DoReset();
  void DoTerminate();

private:
  using SetAtt = QPair<QString, QString>;

  struct Attribute {
    QString name;
    QStringList options;
    bool required;
  };
  using AttList = QList<Attribute>;

  void populateUi();
  void elogSetup(const eudaq::ConfigurationSPC &ini);
  QStringList parseElogCfgLine(const std::string &key);
  QList<SetAtt> attributesSet();
  bool saveCurrentElogSetup();

  Ui::ElogGui *ui;
  AttList mAttributes;
  Elog mElog;
  QDateTime mStartTime, mStopTime;
  QString mConfigFile;
  QString mEventCntConn;
  QSettings mSettings;
  uint mCurrRunN;
  Producer2GUIProxy mProxy;
};

#endif // ELOGRUNCTRL_H
