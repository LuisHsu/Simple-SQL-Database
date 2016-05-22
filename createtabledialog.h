#ifndef CREATETABLEDIALOG_H
#define CREATETABLEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <filesystem.h>

namespace Ui {
class CreateTableDialog;
}

class CreateTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateTableDialog(DBDesc *dbDesc,QWidget *parent = 0);
    ~CreateTableDialog();

private:
    Ui::CreateTableDialog *ui;
    QStandardItemModel *dbModel;
    void showEvent(QShowEvent *);
    DBDesc *dbDesc;
private slots:
    void addColumn(bool);
    void delColumn(bool);
    void createTable();
};

#endif // CREATETABLEDIALOG_H
