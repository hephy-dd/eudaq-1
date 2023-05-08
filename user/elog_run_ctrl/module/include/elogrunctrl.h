#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include "RunControl.hh"
#include "elog.h"

#include <QWidget>
#include <QMap>
#include <QDateTime>
#include <QSettings>

namespace Ui {
class ElogRunCtrl;
}

class ElogRunCtrl : public QWidget, public eudaq::RunControl
{
    Q_OBJECT

public:
    explicit ElogRunCtrl(const std::string &listenaddress = "",QWidget *parent = nullptr);
    ~ElogRunCtrl();

    void Initialise() override;
    void Configure() override;
    void StartRun() override;
    void StopRun() override;
    void Exec() override;
    void Reset() override;
    void Terminate() override;

    static const uint32_t m_id_factory = eudaq::cstr2hash("ElogRC");

private slots:
    void submit(bool autoSubmit = false);

private:
      using SetAtt = QPair<QString, QString>;

    struct Attribute{
        QString name;
        QStringList options;
        bool required;
    };
    using AttList = QList<Attribute>;

    void populateUi();
    void elogSetup();
    QStringList parseElogCfgLine(const std::string &key);
    int eventsCurrRun();
    QList<SetAtt> attributesSet();
    bool saveCurrentElogSetup();

    Ui::ElogRunCtrl *ui;    
    AttList mAttributes;
    Elog mElog;
    QDateTime mStartTime, mStopTime;
    QString mConfigFile;
    QString mEventCntConn;
    QSettings mSettings;
    uint mCurrRunN;
};

#endif // ELOGRUNCTRL_H
