#include "status.h"
#include "ui_status.h"

Status::Status(QWidget *parent) :
    QWidget(parent),
    statusUi(new Ui::Status)
{
    statusUi->setupUi(this);
}

Status::~Status()
{
    delete statusUi;
}
