#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT
public:
    explicit Settings(QWidget *parent = nullptr);

signals:

public slots:

private:
    Ui::Settings *settingsUi;
};

#endif // SETTINGS_H
