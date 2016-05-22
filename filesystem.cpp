#include "filesystem.h"

static QMap<QString,Manipulator *> manipulators;

TableDesc::TableDesc(){
}

TableDesc::~TableDesc()
{

}

void TableDesc::init(QString filepath,int hashCount)
{
    QJsonObject newObj;
    newObj.insert("tables",QJsonArray());
    newObj.insert("hashCount",QJsonValue(hashCount));
    document.setObject(newObj);
    tableFile.setFileName(filepath);
    tableFile.open(QFile::ReadWrite|QFile::Truncate);
    tableFile.close();
}

void TableDesc::load(QString filepath)
{
    tableFile.setFileName(filepath);
    tableFile.open(QFile::ReadOnly);
    QByteArray arr(tableFile.readAll());
    document = QJsonDocument::fromJson(arr);
    tableFile.close();
}

void TableDesc::write()
{
    tableFile.open(QFile::WriteOnly | QFile::Truncate);
    tableFile.write(document.toJson());
    tableFile.close();
}

DBDesc::DBDesc(QString name):name(name),desc(NULL){
}

DBDesc::~DBDesc()
{
    delete desc;
}

void insertData(DBDesc *dbDesc, QString tableName, QString data)
{
    // Get manipulator
    Manipulator *manipulator = NULL;
    if(manipulators.find(tableName) == manipulators.end()){
        manipulators[tableName] = new Manipulator(dbDesc,tableName);
    }
    manipulator = manipulators[tableName];
    int fieldCount = manipulator->fieldNames.count();
    // Insert Data token & Tokenize data
    QStringList tokens = data.split('|');
    QString pairs[fieldCount];
    QString tokenizedData = "";
    for(int i=0; i<tokens.count(); ++i){
        pairs[i] = manipulator->insertToken(tokens.at(i));
        tokenizedData += pairs[i] + "|";
    }
    tokenizedData = tokenizedData.left(tokenizedData.length()-1);
    qDebug() << tokenizedData;
    int dataIndex = manipulator->insertTokenizedData(tokenizedData);
    // Update field document
    for(int i=0; i<fieldCount; ++i){
        QJsonObject docObject = manipulator->fieldDocs[manipulator->fieldNames.at(i)].object();
        QJsonArray arr;
        if(docObject.value(pairs[i]) != QJsonValue::Undefined){
            arr = docObject.value(pairs[i]).toArray();
            docObject.remove(pairs[i]);
        }
        arr.append(QJsonValue(dataIndex));
        docObject.insert(pairs[i],arr);
        manipulator->fieldDocs[manipulator->fieldNames.at(i)].setObject(docObject);
    }
}

Manipulator::Manipulator(DBDesc *dbDesc, QString tableName):name(tableName),dbName(dbDesc->name)
{
    QJsonArray tableArr = dbDesc->desc->document.object().value("tables").toArray();
    QJsonArray fields;
    // Get fields
    QString tablePath = QApplication::applicationDirPath()+"/Databases/"+dbDesc->name+"/"+tableName;
    for(int i=0; i<tableArr.count(); ++i){
        if(tableArr.at(i).toObject().value("name").toString() == name){
            fields = tableArr.at(i).toObject().value("fields").toArray();
            break;
        }
    }
    // Field documents
    foreach (QJsonValue fieldName, fields) {
        fieldNames.push_back(fieldName.toString());
        QFile fieldFile(tablePath+"/"+fieldName.toString()+".field");
        fieldFile.open(QFile::ReadOnly);
        fieldDocs[fieldName.toString()] = QJsonDocument::fromJson(fieldFile.readAll());
        fieldFile.close();
    }
    // Hash files
    for(int i=0; i<dbDesc->desc->document.object().value("hashCount").toInt(); ++i){
        hashFiles.push_back(new QFile(tablePath+"/"+QString::number(i)+".hash"));
    }
    // Data file
    dataFile.setFileName(tablePath+"/data.db");
}

Manipulator::~Manipulator()
{
    foreach (QFile *file, hashFiles) {
        delete file;
    }
}

QString Manipulator::insertToken(QString token)
{
    // Get hash
    int retHash,retOffset = -1;
    retHash = hash33(token,hashFiles.count());
    // Find token
    QFile *hashFile = hashFiles.at(retHash);
    hashFile->open(QFile::ReadWrite);
    QDataStream stream(hashFile);
    while(!stream.atEnd()){
        int dirty,len;
        stream >> dirty;
        stream >> len;
        if(len<1)continue;
        QChar storedToken[len/2+1];
        storedToken[len/2] = '\0';
        stream.readRawData((char *)storedToken,len);
        if(!dirty)continue;
        if(token == QString(storedToken)){
            retOffset = hashFile->pos() - len - 2*sizeof(int);
            break;
        }
    }
    if(retOffset == -1){
        retOffset = hashFile->pos();
        stream << 1;
        stream << token.length()*2;
        stream.writeRawData((char*)token.data(),token.length()*2);
    }
    hashFile->close();
    return QString::number(retHash) + "_" + QString::number(retOffset);
}

int Manipulator::insertTokenizedData(QString data)
{
    int ret = 0;
    bool insert = false;
    dataFile.open(QFile::ReadWrite);
    QDataStream stream(&dataFile);
    while (!stream.atEnd()) {
        int dirty;
        stream >> dirty;
        if(!dirty){
            insert = true;
            break;
        }
        ret++;
        for(int i=0; i<fieldNames.count(); ++i){
            stream >> dirty;
            stream >> dirty;
        }
    }
    if(insert){
        dataFile.seek(dataFile.pos()-4);
    }
    stream << 1;
    foreach (QString tok, data.split('|')) {
        foreach(QString intStr,tok.split('_')){
            stream << intStr.toInt();
        }
    }
    dataFile.close();
    return ret;
}

void cleanManipulator()
{
    foreach (QString key, manipulators.keys()) {
        Manipulator *manipulator = manipulators[key];
        foreach(QString field, manipulator->fieldNames){
            QFile fieldFile(QApplication::applicationDirPath()+"/Databases/"+
                            manipulator->dbName+"/"+
                            manipulator->name+"/"+
                            field+".field");
            fieldFile.open(QFile::WriteOnly | QFile::Truncate);
            fieldFile.write(manipulator->fieldDocs[field].toJson());
            fieldFile.close();
        }
        delete manipulator;
    }
    manipulators.clear();
}

int hash33(QString key, int bucketCount)
{
    unsigned int hv = 0;
    foreach(QChar c, key){
        hv = (hv << 5) + hv + c.unicode();
    }
    return hv % bucketCount;
}
