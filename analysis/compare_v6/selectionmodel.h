#ifndef SELECTIONMODEL_H
#define SELECTIONMODEL_H

#include <QStandardItemModel>
#include <QMimeData>

class SelectionModel : public QStandardItemModel
{
    Q_OBJECT
public:
    using QStandardItemModel::QStandardItemModel;
    Qt::DropActions supportedDropActions()const override;
    QStringList mimeTypes() const override;
    QMimeData * mimeData(const QModelIndexList &indexes) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
signals:

public slots:
};

#endif // SELECTIONMODEL_H
