#include "treeitem.h"
#include <QDebug>


TreeItem::TreeItem(const QVector<QVariant> &data, TreeItem *parent)
    : m_itemData(data), m_parentItem(parent)
{}
TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
    m_childItems.append(item);
}

TreeItem *TreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::columnCount() const
{
    return m_itemData.count();
}

QVariant TreeItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QString();
    return m_itemData.at(column);
}

int TreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

const TreeItem* TreeItem::parentItem()const{
    return m_parentItem;
}

const QVector<TreeItem*> TreeItem::childItems()const{
    return m_childItems;
}

const QVector<QVariant> TreeItem::itemData() const{
    return m_itemData;
}

void TreeItem::setItemData(QVector<QVariant> data){
    m_itemData = data;
}

void TreeItem::setParentItem(TreeItem *parentItem){
    m_parentItem = parentItem;
}

void serialize(QDataStream &ds, const TreeItem &root, int depth = 0){
    QVector<QVariant> data = root.itemData();
    ds << depth;
    ds << data;
    //qDebug() << depth;
    //qDebug() << data;
    for (TreeItem *item : root.childItems()){
        serialize(ds,*item,depth+1);
    }
}

QDataStream &operator << (QDataStream &ds, const TreeItem &inObj){
    serialize(ds, inObj);
    return ds;
}

void TreeItem::probe(QByteArray& block){
    QDataStream ds(&block,QIODevice::ReadWrite);
    unsigned depth = 0;
    QVector<QVariant> data;
    while(!ds.atEnd()){
        ds>>depth;
        ds>>data;
        qDebug() << depth;
        qDebug() << data;
        depth = 0;
        data.clear();
    }
}

void fillRootItem(QDataStream &ds, TreeItem &root){
    if (ds.atEnd()){
        return;
    }
    qDebug() << "fill root item";
    int depth;
    int currentDepth = 0;
    ds >> depth;
    currentDepth = depth; // currentDepth holds the depth at which p points
    QVector<QVariant> data;
    ds >> data;
    root.setItemData(data);

    if (ds.atEnd()){
        return;
    }
    TreeItem *parent = &root;
    TreeItem *p = new TreeItem(QVector<QVariant>(),nullptr);
    p->setParentItem(parent);
    parent->appendChild(p);
    ds >> depth;
    currentDepth = depth;
    data.clear();
    ds >> data;
    p->setItemData(data);

    // FINISHED INITIALISING

    while(!ds.atEnd()){
        data.clear();
        ds >> depth;
        if (depth < 0){
            return;
        }
        ds >> data;
        if (depth == currentDepth){
            p = new TreeItem(data,parent);
            parent->appendChild(p);
        }
        if (depth < currentDepth){
            for (unsigned i = 0; i < currentDepth-depth; i++){
                parent = const_cast<TreeItem*>(parent->parentItem());
            }
            p = new TreeItem(data,parent);
            parent->appendChild(p);
        }
        if (depth > currentDepth){
            parent = p;
            p = new TreeItem(data,parent);
            parent->appendChild(p);
        }
        currentDepth = depth;
    }
}

QDataStream &operator >> (QDataStream &ds, TreeItem &outObj){
    fillRootItem(ds, outObj);
    return ds;
}
