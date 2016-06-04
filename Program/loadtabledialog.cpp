#include "loadtabledialog.h"
#include "ui_loadtabledialog.h"

LoadTableDialog::LoadTableDialog(DBDesc *dbDesc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadTableDialog),
    dbDesc(dbDesc)
{
    ui->setupUi(this);
    connect(ui->buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(loadTable()));
    connect(ui->toolButton,SIGNAL(clicked(bool)),this,SLOT(chooseFile(bool)));
}

LoadTableDialog::~LoadTableDialog()
{
    delete ui;
}

void LoadTableDialog::chooseFile(bool)
{
    QFileDialog fDialog(this);
    fDialog.setFilter(QDir::NoDotAndDotDot | QDir::Files);
    fDialog.exec();
    if(fDialog.result() == QFileDialog::Accepted){
        filepath = fDialog.selectedFiles().at(0);
        ui->lineEdit_2->setText(QFileInfo(filepath).fileName());
    }
}

void LoadTableDialog::loadTable()
{
    QString tableName = ui->NameLine->text();
    if(tableName.isEmpty()){
        QMessageBox::warning(this,"錯誤","名稱不可為空");
        return;
    }
    if(ui->lineEdit_2->text().isEmpty()){
        QMessageBox::warning(this,"錯誤","請選擇檔案");
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

    // Load field
    QFile insertedFile(filepath);
    insertedFile.open(QFile::ReadOnly);
    QTextStream textStream(insertedFile.readAll());
    QString fieldLine = textStream.readLine();
    fieldLine = fieldLine.remove("/* ").remove(" */");
    // Update fields
    QJsonArray tables = dbDesc->desc->document.object().value("tables").toArray();
    QJsonObject newTable;
    newTable.insert("name",QJsonValue(tableName));
    QJsonArray fields;
    foreach(QString fieldName,fieldLine.split('|')){
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
    // Insert data
    while(!textStream.atEnd()){
        QString line = textStream.readLine();
        if(!line.isEmpty()){
            insertData(dbDesc, tableName, line);
        }
    }
}
