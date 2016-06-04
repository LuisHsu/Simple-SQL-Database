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
    connect(ui->DBListWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(update_manipulators(QListWidgetItem*)));
    connect(ui->TableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(update_fields(int)));
    connect(ui->QueryButton,SIGNAL(clicked(bool)),this,SLOT(query_click(bool)));

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
    // Set Lexer
    sqlLexer = new QsciLexerSQL;
    QFont font;
    font.setPointSize(12);
    sqlLexer->setDefaultFont(font);
    sqlLexer->setAutoIndentStyle(1);
    ui->textEdit->setLexer(sqlLexer);
    ui->textEdit->setWrapMode(QsciScintilla::WrapWord);
    ui->textEdit->setMarginsFont(font);
    ui->textEdit->setMarginWidth(0,QFontMetrics(font).width("000")+6);
    ui->textEdit->setMarginLineNumbers(0,true);
    // Set result table
    ui->ResultView->setModel(new QStandardItemModel);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete sqlLexer;
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

void MainWindow::update_manipulators(QListWidgetItem *item)
{
    if(!ui->DBListWidget->selectedItems().isEmpty()){
        cleanManipulator();
        foreach (DBDesc *dbDesc, dbList) {
            if(dbDesc->name == item->text()){
                foreach (QJsonValue table, dbDesc->desc->document.object().value("tables").toArray()) {
                    QString tableName = table.toObject().value("name").toString();
                    loadManipulator(dbDesc,tableName);
                }
                break;
            }
        }
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

void MainWindow::query_click(bool)
{
    QueryExecuter executer;
    ui->ResultTextbrowser->setText(query(ui->textEdit->text(),executer)+"</br>");
    QStandardItemModel *resModel = (QStandardItemModel *)ui->ResultView->model();
    resModel->clear();
    resModel->setHorizontalHeaderLabels(executer.fieldList);
    for(int i=0; i<executer.datas.size(); ++i){
        resModel->insertRow(i);
        for(int j=0; j<executer.datas.at(i).size(); ++j){
            resModel->setData(resModel->index(i,j),executer.datas.at(i).at(j));
        }
    }
    ui->ResultView->resizeColumnsToContents();
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
