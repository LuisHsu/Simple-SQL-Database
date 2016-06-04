#include "createdbdialog.h"
#include "ui_createdbdialog.h"

CreateDBDialog::CreateDBDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateDBDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox,SIGNAL(accepted()),this,SLOT(saveClick()));
    connect(ui->buttonBox,SIGNAL(rejected()),this,SLOT(reject()));
}

CreateDBDialog::~CreateDBDialog()
{
    delete ui;
}

void CreateDBDialog::saveClick()
{
    if(ui->lineEdit_2->text().isEmpty()){
        QMessageBox::warning(this,"錯誤","名稱不可為空");
        return;
    }
    // Check DB name
    dbDesc = new DBDesc(ui->lineEdit_2->text());

    // Make & ensure DB directory
    QString dbPath = QApplication::applicationDirPath()+"/Databases/"+dbDesc->name;
    if(!QDir(dbPath).exists()){
        QDir().mkdir(dbPath);
    }else{
        QMessageBox::warning(this,"錯誤","資料庫已存在");
        return;
    }
    // Create table description file
    dbDesc->desc = new TableDesc;
    dbDesc->desc->init(dbPath + "/Table.desc",ui->spinBox->value());
    dbDesc->desc->write();

    accept();
}
