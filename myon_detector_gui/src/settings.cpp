#include <settings.h>
#include <ui_settings.h>
#include <../shared/ubx_msg_key_name_map.h>

Settings::Settings(QWidget *parent) : QDialog(parent),
settingsUi(new Ui::Settings)
{
	settingsUi->setupUi(this);
}

void Settings::addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates) {
	for (QMap<uint16_t, int>::iterator it = ubxMsgRates.begin(); it != ubxMsgRates.end(); it++) {
		UbxMsgRateListItem *item = new UbxMsgRateListItem(settingsUi->ubloxSignalStates);
		item->setCheckState(Qt::CheckState::Checked);
		item->key = it.key();
		item->rate = it.value();
		item->setFlags(item->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
		item->setFlags(item->flags() | Qt::ItemIsEditable);
		item->name = ubxMsgKeyNameMap.value(it.key());
		item->setText(QString(QString::number(item->rate).rightJustified(3, '0') + " " + item->name));
		settingsUi->ubloxSignalStates->addItem(item);
	}
}
