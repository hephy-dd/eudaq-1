#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include "RunControl.hh"
#include <QWidget>

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

    static const uint32_t m_id_factory = eudaq::cstr2hash("ElogRC");

private:
    Ui::ElogRunCtrl *ui;
};

#endif // ELOGRUNCTRL_H
