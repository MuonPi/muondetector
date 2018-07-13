#include <settings.h>
#include <ui_settings.h>

Settings::Settings(QWidget *parent) : QDialog(parent),
  settingsUi(new Ui::Settings)
{
    settingsUi->setupUi(this);
}
