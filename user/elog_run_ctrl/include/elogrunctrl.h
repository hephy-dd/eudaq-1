#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include <QWidget>

namespace Ui {
class ElogRunCtrl;
}

class ElogRunCtrl : public QWidget
{
    Q_OBJECT

public:
    explicit ElogRunCtrl(QWidget *parent = nullptr);
    ~ElogRunCtrl();

private:
    Ui::ElogRunCtrl *ui;
};

#endif // ELOGRUNCTRL_H
