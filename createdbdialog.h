#ifndef CREATEDBDIALOG_H
#define CREATEDBDIALOG_H

#include <QDialog>
#include <QDir>
#include <QMessageBox>
#include <filesystem.h>

namespace Ui {
class CreateDBDialog;
}

class CreateDBDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateDBDialog(QWidget *parent = 0);
    ~CreateDBDialog();
    DBDesc *dbDesc;

private:
    Ui::CreateDBDialog *ui;

private slots:
    void saveClick();
};

#endif // CREATEDBDIALOG_H
