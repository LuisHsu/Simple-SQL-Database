#include <QCoreApplication>
#include <QDir>

#include <../Program/filesystem.h>

int main(int argc, char *argv[])
{
    QCoreApplication *a = new QCoreApplication(argc,argv);
    // Create dump directory
    if(!QDir(QCoreApplication::applicationDirPath()+"/Dumps").exists()){
        QDir().mkdir(QCoreApplication::applicationDirPath()+"/Dumps");
    }
    // Load Databases
    foreach (QString database, QDir(QCoreApplication::applicationDirPath()+"/Databases").entryList(QDir::NoDotAndDotDot|QDir::Dirs)) {
        DBDesc dbDesc(database);
        dbDesc.desc = new TableDesc;
        dbDesc.desc->load(QCoreApplication::applicationDirPath()+"/Databases/"+database+"/Table.desc");
        QMap<QString,Manipulator*> *manipulators = getManipulator();
        foreach (QJsonValue table, dbDesc.desc->document.object().value("tables").toArray()) {
            QString tableName = table.toObject().value("name").toString();
            loadManipulator(&dbDesc,tableName);
            Manipulator *manipulator = (*manipulators)[tableName];
            foreach (QString field, manipulator->fieldNames) {
                QFile dumpFile(QCoreApplication::applicationDirPath()+"/Dumps/"+tableName+"_"+field+".txt");
                dumpFile.open(QFile::WriteOnly|QFile::Truncate);
                QTextStream stream(&dumpFile);
                QStringList buckets[manipulator->hashFiles.size()];
                foreach (QString attribute, manipulator->fieldDocs[field].object().keys()) {
                    buckets[attribute.split("_").at(0).toInt()].push_back(tokenToString(tableName,attribute));
                }
                for(int i=0; i<manipulator->hashFiles.size(); ++i){
                    stream << "Bucket "+QString::number(i)+"\t";
                    QString res = "";
                    foreach (QString attribute, buckets[i]) {
                        res += attribute+",";
                    }
                    res.chop(1);
                    stream << res + "\n";
                }
                dumpFile.close();
            }
        }
        cleanManipulator();
    }
    delete a;
    return 0;
}
