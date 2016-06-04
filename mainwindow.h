#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QListWidgetItem>
#include <QDir>
#include <QDebug>
#include <QStandardItemModel>
#include <createdbdialog.h>
#include <createtabledialog.h>
#include <filesystem.h>
#include <loadtabledialog.h>
#include <Qsci/qscilexersql.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QList<DBDesc *> dbList;
    QsciLexerSQL *sqlLexer;

private slots:
    void db_create_slot(bool);
    void db_delete_slot(bool);
    void table_create_slot(bool);
    void table_load_slot(bool);
    void update_tables(int row);
    void update_manipulators(QListWidgetItem *item);
    void update_fields(int row);
    void query_click(bool);
};

#endif // MAINWINDOW_H
