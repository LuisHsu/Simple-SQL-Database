#include "createtabledialog.h"
#include "ui_createtabledialog.h"

CreateTableDialog::CreateTableDialog(DBDesc *dbDesc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateTableDialog),
    dbDesc(dbDesc)
{
    ui->setupUi(this);
    connect(ui->buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
    connect(ui->addButton,SIGNAL(clicked(bool)),this,SLOT(addColumn(bool)));
    connect(ui->deleteButton,SIGNAL(clicked(bool)),this,SLOT(delColumn(bool)));
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(createTable()));
}

CreateTableDialog::~CreateTableDialog()
{
    delete ui;
    delete dbModel;
}

void CreateTableDialog::showEvent(QShowEvent *)
{

    dbModel = new QStandardItemModel(0,2,ui->tableView);
    QStringList vHeader;
    vHeader << "欄位名稱";
    dbModel->setHorizontalHeaderLabels(vHeader);
    ui->tableView->setModel(dbModel);
    ui->tableView->setColumnWidth(0,ui->tableView->width());
}

void CreateTableDialog::addColumn(bool)
{
    QList<QStandardItem *> rowList;
    rowList.push_back(new QStandardItem(""));
    dbModel->appendRow(rowList);
}

void CreateTableDialog::delColumn(bool)
{
    foreach (QModelIndex index, ui->tableView->selectionModel()->selection().indexes()) {
        dbModel->removeRow(index.row());
    }
}

void CreateTableDialog::createTable()
{
    QString tableName = ui->lineEdit->text();
    if(tableName.isEmpty()){
        QMessageBox::warning(this,"錯誤","名稱不可為空");
        return;
    }

    // Create table directory
    QDir tableDir(QApplication::applicationDirPath()+"/Databases/"+dbDesc->name+"/"+tableName);
    if(tableDir.exists()){
        QMessageBox::warning(this,"錯誤","資料表已存在");
        return;
    }else{
        QDir().mkdir(tableDir.absolutePath());
    }

    // Update fields
    QJsonArray tables = dbDesc->desc->document.object().value("tables").toArray();
    QJsonObject newTable;
    newTable.insert("name",QJsonValue(tableName));
    QJsonArray fields;
    for(int i=0; i<dbModel->rowCount(); ++i){
        QString fieldName = dbModel->item(i,0)->text();
        fields.append(QJsonValue(fieldName));
        QFile fieldFile(tableDir.absolutePath()+"/"+fieldName+".field");
        fieldFile.open(QFile::WriteOnly | QFile::Truncate);
        QJsonDocument fieldDoc;
        fieldDoc.setObject(QJsonObject());
        fieldFile.write(fieldDoc.toJson());
        fieldFile.close();
    }
    newTable.insert("fields",fields);
    tables.append(newTable);
    QJsonObject mainObj = dbDesc->desc->document.object();
    mainObj.insert("tables",tables);
    dbDesc->desc->document.setObject(mainObj);
    dbDesc->desc->write();

    // Create data & hash content
    QFile dataFile(tableDir.absolutePath()+"/data.db");
    dataFile.open(QFile::WriteOnly | QFile::Truncate);
    dataFile.close();
    for(int i=0; i<dbDesc->desc->document.object().value("hashCount").toInt(); ++i){
        dataFile.setFileName(tableDir.absolutePath()+"/"+QString::number(i,16)+".hash");
        dataFile.open(QFile::WriteOnly | QFile::Truncate);
        dataFile.close();
    }

    accept();
}
