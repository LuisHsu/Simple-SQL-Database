#ifndef LOADTABLEDIALOG_H
#define LOADTABLEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <filesystem.h>
#include <QDebug>

namespace Ui {
class LoadTableDialog;
}

class LoadTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadTableDialog(DBDesc *dbDesc, QWidget *parent = 0);
    ~LoadTableDialog();

private:
    Ui::LoadTableDialog *ui;
    DBDesc *dbDesc;
    QString filepath;

private slots:
    void chooseFile(bool);
    void loadTable();

};

#endif // LOADTABLEDIALOG_H
