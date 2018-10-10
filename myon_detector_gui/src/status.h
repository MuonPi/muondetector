#ifndef STATUS_H
#define STATUS_H

#include <QWidget>

namespace Ui {
class Status;
}

class Status : public QWidget
{
    Q_OBJECT

public:
    explicit Status(QWidget *parent = nullptr);
    ~Status();

private:
    Ui::Status *statusUi;
};

#endif // STATUS_H
