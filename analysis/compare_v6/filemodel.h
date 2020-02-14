#ifndef FILEMODEL_H
#define FILEMODEL_H
#include <QFileSystemModel>
#include <treeitem.h>

class FileModel : public QFileSystemModel
{
    Q_OBJECT
public:
    using QFileSystemModel::QFileSystemModel;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
private:
    void createTree(QString path, TreeItem &root)const;
};

#endif // FILEMODEL_H
