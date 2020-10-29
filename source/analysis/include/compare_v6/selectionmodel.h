#ifndef SELECTIONMODEL_H
#define SELECTIONMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include <treeitem.h>

class SelectionModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    //using QAbstractItemModel::QAbstractItemModel;
    explicit SelectionModel(const QString &data, QObject *parent);
    ~SelectionModel()override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::DropActions supportedDropActions()const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QStringList mimeTypes() const override;
    QMimeData * mimeData(const QModelIndexList &indexes) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
signals:
    void movedActionFinished();
public slots:
    void onMovedActionFinished();
private:
    void setupModelData(const QStringList &lines, TreeItem *parent);
    QModelIndexList lastMoved;
    TreeItem *rootItem;
};

#endif // SELECTIONMODEL_H
