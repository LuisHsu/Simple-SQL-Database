#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->db_create_action,SIGNAL(triggered(bool)),this,SLOT(db_create_slot(bool)));
    connect(ui->db_delete_action,SIGNAL(triggered(bool)),this,SLOT(db_delete_slot(bool)));
    connect(ui->table_create_action,SIGNAL(triggered(bool)),this,SLOT(table_create_slot(bool)));
    connect(ui->table_load_action,SIGNAL(triggered(bool)),this,SLOT(table_load_slot(bool)));
    connect(ui->DBListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(update_tables(int)));
    connect(ui->TableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(update_fields(int)));

    // Load Databases
    QDir dbRoot(QApplication::applicationDirPath()+"/Databases");
    if(dbRoot.exists()){
        foreach (QString dbName, dbRoot.entryList(QDir::NoDotAndDotDot|QDir::AllDirs)) {
            DBDesc *dbDesc = new DBDesc(dbName);
            dbDesc->desc = new TableDesc;
            dbDesc->desc->load(QApplication::applicationDirPath()+"/Databases/"+dbName+"/Table.desc");
            dbList.push_back(dbDesc);
            QListWidgetItem *newItem = new QListWidgetItem;
            newItem->setText(dbName);
            ui->DBListWidget->addItem(newItem);
        }
    }else{
        QDir().mkdir(QApplication::applicationDirPath()+"/Databases");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    foreach (DBDesc *db, dbList) {
        delete db;
    }
}

void MainWindow::update_tables(int row)
{
    while (ui->TableListWidget->count() > 0) {
        delete ui->TableListWidget->takeItem(0);
    }
    if(row < 0){
        return;
    }
    cleanManipulator();
    foreach (QJsonValue table, dbList.at(row)->desc->document.object().value("tables").toArray()) {
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(table.toObject().value("name").toString());
        ui->TableListWidget->addItem(newItem);
    }
}

void MainWindow::update_fields(int row)
{
    while (ui->FieldListWidget->count() > 0) {
        delete ui->FieldListWidget->takeItem(0);
    }
    if(row < 0){
        return;
    }
    QJsonObject table = dbList.at(ui->DBListWidget->currentRow())->desc->document.object().value("tables").toArray().at(row).toObject();
    foreach (QJsonValue field, table.value("fields").toArray()) {
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(field.toString());
        ui->FieldListWidget->addItem(newItem);
    }
}

void MainWindow::db_create_slot(bool)
{
    CreateDBDialog dbdialog;
    dbdialog.exec();
    if(dbdialog.result() == QDialog::Accepted){
        dbList.push_back(dbdialog.dbDesc);
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(dbdialog.dbDesc->name);
        ui->DBListWidget->addItem(newItem);
    }
}

void MainWindow::db_delete_slot(bool)
{
    if(!ui->DBListWidget->selectedItems().isEmpty()){
        QMessageBox confirmBox;
        confirmBox.setText("確定要刪除資料庫 " + ui->DBListWidget->currentItem()->text() + " ?");
        confirmBox.setStandardButtons(QMessageBox::Cancel|QMessageBox::Ok);
        confirmBox.setDefaultButton(QMessageBox::Cancel);
        confirmBox.exec();
        if(confirmBox.result() == QMessageBox::Ok){
            int index = ui->DBListWidget->currentRow();
            QListWidgetItem *delDB = ui->DBListWidget->takeItem(index);
            delete dbList.at(index);
            dbList.removeAt(index);
            QDir(QApplication::applicationDirPath()+"/Databases/"+delDB->text()).removeRecursively();
            delete delDB;
        }
        update_fields(-1);
        update_tables(-1);
    }else{
        QMessageBox::warning(this,"錯誤","沒有選取資料庫");
    }
}

void MainWindow::table_create_slot(bool)
{
    if(!ui->DBListWidget->selectedItems().isEmpty()){
        CreateTableDialog tbDialog(dbList.at(ui->DBListWidget->currentRow()),this);
        tbDialog.exec();
        update_tables(ui->DBListWidget->currentRow());
    }else{
        QMessageBox::warning(this,"錯誤","沒有選取資料庫");
    }
}

void MainWindow::table_load_slot(bool)
{
    if(!ui->DBListWidget->selectedItems().isEmpty()){
        LoadTableDialog tbDialog(dbList.at(ui->DBListWidget->currentRow()),this);
        tbDialog.exec();
        update_tables(ui->DBListWidget->currentRow());
    }else{
        QMessageBox::warning(this,"錯誤","沒有選取資料庫");
    }
}
