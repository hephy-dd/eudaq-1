#include "elogrunctrl.h"
#include "ui_elogrunctrl.h"

ElogRunCtrl::ElogRunCtrl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ElogRunCtrl)
{
    ui->setupUi(this);
}

ElogRunCtrl::~ElogRunCtrl()
{
    delete ui;
}
