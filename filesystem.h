#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QFile>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QDataStream>
#include <QMap>
#include <QDebug>

class TableDesc{
public:
    TableDesc();
    ~TableDesc();
    void init(QString filepath, int hashCount);
    void load(QString filepath);
    void write();
    QFile tableFile;
    QJsonDocument document;
};

class DBDesc{
public:
    DBDesc(QString name);
    ~DBDesc();
    QString name;
    TableDesc *desc;
};

void insertData(DBDesc *dbDesc, QString tableName, QString data);
void cleanManipulator();
int hash33(QString key, int bucketCount);

class Manipulator{
public:
    Manipulator(DBDesc *dbDesc, QString tableName);
    ~Manipulator();
    QString name,dbName;
    QList<QString> fieldNames;
    QMap<QString,QJsonDocument> fieldDocs;
    QList<QFile *> hashFiles;
    QFile dataFile;
    QString insertToken(QString token);
    int insertTokenizedData(QString data);
};


#endif // FILESYSTEM_H
