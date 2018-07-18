#include <settings.h>
#include <ui_settings.h>

Settings::Settings(QWidget *parent) : QDialog(parent),
  settingsUi(new Ui::Settings)
{
    settingsUi->setupUi(this);
    // this is how to insert a new item to the listWidget
//    QListWidgetItem *item = new QListWidgetItem(settingsUi->ubloxSignalStates);
//    item->setCheckState(Qt::CheckState::PartiallyChecked);
//    settingsUi->ubloxSignalStates->addItem(item);
}
