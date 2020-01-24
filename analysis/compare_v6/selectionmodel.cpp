#include "selectionmodel.h"
#include <QAbstractItemModel>
#include <QStandardItem>
#include <QDataStream>
#include <QIcon>
#include <QDebug>

SelectionModel::SelectionModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent){
    rootItem = new TreeItem({tr("Title"), tr("Summary")});
    setupModelData(data.split('\n'), rootItem);
    connect(this, &SelectionModel::movedActionFinished, this, &SelectionModel::onMovedActionFinished);
}

SelectionModel::~SelectionModel(){
    delete rootItem;
}

Qt::DropActions SelectionModel::supportedDropActions()const{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags SelectionModel::flags(const QModelIndex &index) const{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);//QFileSystemModel::flags(index);

    if (!index.isValid())
    {
        return Qt::NoItemFlags;//Qt::ItemIsDropEnabled | defaultFlags;
    }else{
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    }
}

QStringList SelectionModel::mimeTypes() const{
    QStringList types;
    types << "application/vnd.text.list";
    //types << "text/uri-list";
    types << "text/plain";
    types << "application/x-dndicondata";
    //qDebug("standarditemmodel mimeTypes ausgefuehrt");

    return types;
}

QMimeData *SelectionModel::mimeData(const QModelIndexList &indexes)const{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    /*for (auto index : indexes){

    }
    if (index.isValid()){

    }*/
    TreeItem *item = static_cast<TreeItem*>(indexes.at(0).internalPointer());
    stream << *item;
    mimeData->setData("text/plain",encodedData);
    return mimeData;
}

bool SelectionModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column, const QModelIndex &parent){
    //qDebug("I am in the model");
    if (action==Qt::IgnoreAction){
        return true;
    }
    //qDebug("not ignored");
    //if (!data->hasFormat("application/vnd.text.list"))
    if (!data->hasFormat("text/plain"))
    {
        return false;
    }
    //qDebug("not wrong Format");
    if (column > 0)
    {
        return false;
    }
    QByteArray encodedData = data->data("text/plain");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    TreeItem::probe(encodedData);
    //while (!stream.atEnd()){
        qDebug() << "drop";
        TreeItem *p = static_cast<TreeItem*>(parent.internalPointer());
        TreeItem *item = new TreeItem(QVector<QVariant>(),p);
        p->appendChild(item);
        stream >> *item;
        emit dataChanged(parent,parent);
    //}
    return false;
}

void SelectionModel::onMovedActionFinished(){
    qDebug() << lastMoved;
    for (auto index : lastMoved){
        removeRow(index.row(),index.parent());
    }
}

int SelectionModel::columnCount(const QModelIndex &parent) const{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant SelectionModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

QVariant SelectionModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex SelectionModel::index(int row, int column, const QModelIndex &parent) const{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex SelectionModel::parent(const QModelIndex &index) const{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = const_cast<TreeItem*>(childItem->parentItem());

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int SelectionModel::rowCount(const QModelIndex &parent) const{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void SelectionModel::setupModelData(const QStringList &lines, TreeItem *parent){
    QVector<TreeItem*> parents;
    QVector<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            position++;
        }

        const QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            const QStringList columnStrings = lineData.split('\t', QString::SkipEmptyParts);
            QVector<QVariant> columnData;
            columnData.reserve(columnStrings.count());
            for (const QString &columnString : columnStrings)
                columnData << columnString;

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new TreeItem(columnData, parents.last()));
        }
        ++number;
    }
    /*
    QByteArray block;
    QDataStream stream(&block,QIODevice::ReadWrite);
    stream << *rootItem;
    qDebug() << block.size();
    qDebug() << "probe";
    stream.unsetDevice();
    qDebug() << block.size();
    QDataStream stream2(&block, QIODevice::ReadOnly);
    TreeItem *tree = new TreeItem(QVector<QVariant>(),nullptr);
    stream2 >> *tree;
    QByteArray block2;
    QDataStream stream3(&block2,QIODevice::ReadWrite);
    stream3 << *tree;
    qDebug() << block2.size();
    qDebug() << (block == block2);
    */
}
