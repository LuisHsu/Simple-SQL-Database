#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QFile>
#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QMap>
#include <QRegularExpression>
#include <QDebug>
#include <QPair>

class TableDesc{
public:
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

class Manipulator{
public:
    Manipulator(DBDesc *dbDesc, QString tableName);
    ~Manipulator();
    QString name,dbName;
    QStringList fieldNames;
    QMap<QString,QJsonDocument> fieldDocs;
    QList<QFile *> hashFiles;
    QFile dataFile;
    QString insertToken(QString token);
    int insertTokenizedData(QString data);
};

class QueryLexer{
public:
    QueryLexer();
    QString lex(QString qryStr);
    QString check();
    QStringList fields,tables,joins,conditions;
    bool isDistinct,isError,allField,allTable;
};

typedef enum{RESULT,EQUAL,BRACKET,LOGICAL,FIELD,CONSTENT} NodeType;
typedef enum{EQU,OR,AND,RET} Operation;

class ParserNode{
public:
    NodeType type;
    QString value;
};

class Instruction{
public:
    Operation operation;
    ParserNode regs[3];
};

class QueryParser{
public:
    virtual QString parse(QStringList &conditions);
    virtual QString check(QueryLexer &lexer);
    bool isError;
    QStringList fields;
    QList<Instruction>instructions;
protected:
    QList<ParserNode>datas,operators;
};

class JoinParser:public QueryParser{
public:
    QString parse(QStringList &joins);
    QString check(QueryLexer &);
};

class QueryExecuter{
public:
    bool isError;
    QStringList fieldList;
    QList<QStringList> datas;
    QStringList tables;
    QString execute(QueryLexer &lex, JoinParser &joinParser, QueryParser &condParser);
};

void insertData(DBDesc *dbDesc, QString tableName, QString data);
void cleanManipulator();
QMap<QString,Manipulator *> *getManipulator();
void loadManipulator(DBDesc *dbDesc, QString tableName);
QString query(QString qryStr, QueryExecuter &executer);
int hash33(QString key, int bucketCount);
QString tokenToString(QString table, QString token);
QString stringToToken(QString table, QString str);

#endif // FILESYSTEM_H
