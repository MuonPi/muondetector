#include "delegates/itemdeletedelegate.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>

DeleteItemDelegate::DeleteItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {
}

QRect DeleteItemDelegate::trashRect(const QStyleOptionViewItem& option) const {
    constexpr int size = 16;
    constexpr int margin = 6;

    return QRect(option.rect.right() - size - margin, option.rect.center().y() - size / 2, size,
                 size);
}

void DeleteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // draw normal item
    QStyledItemDelegate::paint(painter, opt, index);

    // draw trash icon
    QIcon icon = QApplication::style()->standardIcon(QStyle::SP_TrashIcon);

    icon.paint(painter, trashRect(option));
}

bool DeleteItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                     const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);

        if (trashRect(option).contains(mouseEvent->pos())) {
            model->removeRow(index.row());
            return true; // swallow event
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}