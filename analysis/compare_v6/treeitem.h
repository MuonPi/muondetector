#ifndef TREEITEM_H
#define TREEITEM_H
#include <QVector>
#include <QString>
#include <QDataStream>
#include <QByteArray>

 class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    friend QDataStream &operator << (QDataStream &ds, const TreeItem &inObj);
    friend QDataStream &operator >> (QDataStream &ds,  TreeItem &outObj);
    const TreeItem *parentItem()const;
    const QVector<TreeItem*> childItems()const;
    const QVector<QVariant> itemData()const;
    void setItemData(QVector<QVariant> data);
    void setParentItem(TreeItem *parentItem);
    static void probe(QByteArray& block);
private slots:
     void onMovedItemsFinished();
private:
    QVector<QVariant> m_itemData; // 0: (QString) name; 1: (QString) size; 2: (QString) path; 3: (QIcon) icon
    QVector<TreeItem*> m_childItems;
    TreeItem *m_parentItem;
};

#endif // TREEITEM_H
