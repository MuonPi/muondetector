#include "filemodel.h"
#include <QMimeData>
#include <QDebug>
#include <QFileSystemModel>
#include <QDirIterator>
#include <treeitem.h>

QStringList FileModel::mimeTypes() const
{
    QStringList types;
    //types << "application/vnd.text.list";
    //types << "text/uri-list";
    types << "text/plain";
    types << "application/x-dndicondata";
    //qDebug("standarditemmodel mimeTypes ausgefuehrt");

    return types;
}

void FileModel::createTree(QString rootPath, TreeItem &root)const{
    // should create a Tree with every file and every subfolder in it
    QDirIterator iter(rootPath, QDirIterator::Subdirectories);
    QVector<QVariant> data;
    QStringList iterPath = rootPath.split("/");
    data.append(QVariant::fromValue(rootPath.split("/").last()));
    data.append(QVariant::fromValue(rootPath));
    TreeItem *parent = &root;
    int depth = 0;
    while (iter.hasNext()){
        iter.next();
        QStringList path = iter.filePath().split("/");
        if (path.last()=="." || path.last()==".."){
            continue;
        }
        data.clear();
        data.append(QVariant::fromValue(path.last()));
        data.append(QVariant::fromValue(iter.filePath()));
        if (path.size()-iterPath.size()-1 > depth){
            parent = parent->childItems().last();
        }
        for (int i = 0; i < depth - path.size()+iterPath.size()+1; i++){
            parent = const_cast<TreeItem*>(parent->parentItem());
        }
        TreeItem *p = new TreeItem(data,parent);
        parent->appendChild(p);
        qDebug() << depth << path.size() << iterPath.size();
        depth = path.size()-iterPath.size()-1;
    }
    qDebug() << "end of create tree";
    //qDebug() << type(path) << fileName(path) << filePath(path) << fileInfo(path) << fileIcon(path);
}

QMimeData *FileModel::mimeData(const QModelIndexList &indexes)const{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    //QByteArray encodedIcon;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    //QDataStream stream2(&encodedIcon,QIODevice::WriteOnly);
    //for (QModelIndex index : indexes) {
    if (indexes.at(0).isValid()) {
        //QIcon icon = fileIcon(index);
        //stream2 << icon;
        //QString text = data(index, Qt::DisplayRole).toString();
        //stream << text;
        //text = filePath(index);//data(index, Qt::UserRole).toString();
        //qDebug()<<data(index,Qt::UserRole);
        //stream << text;
        TreeItem item(QVector<QVariant>(),nullptr);
        createTree(filePath(indexes.at(0)),item);
        qDebug() << "before stream";
        stream << item;
        qDebug() << "after stream";
        //TreeItem::probe(encodedData);
    }
    //}
    //mimeData->setData("application/vnd.text.list", encodedData);
    mimeData->setData("text/plain", encodedData);
    //qDebug("filesystemmodel mimeData was set");
    return mimeData;
}

Qt::DropActions FileModel::supportedDropActions()const{
    return Qt::CopyAction | Qt::MoveAction;
}
