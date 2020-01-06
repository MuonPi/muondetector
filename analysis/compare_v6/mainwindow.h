#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <filemodel.h>
#include <QPointer>
#include <selectionmodel.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void customContextMenu(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    QString workDirectory;
    QPointer<FileModel> fileModel;
    QPointer<SelectionModel> selectionModel;
};
#endif // MAINWINDOW_H
