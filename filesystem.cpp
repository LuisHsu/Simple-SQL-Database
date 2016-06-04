#include "filesystem.h"

static QMap<QString,Manipulator *> manipulators;

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

QString query(QString qryStr, QueryExecuter &executer)
{
    // Lex
    QueryLexer lexer;
    QString errStr = lexer.lex(qryStr);
    errStr += lexer.check();
    if(lexer.isError){
        return errStr;
    }
    // Parse joins
    JoinParser joinParser;
    errStr += joinParser.parse(lexer.joins);
    if(joinParser.isError){
        return errStr;
    }
    // Check joins
    errStr += joinParser.check(lexer);
    if(joinParser.isError){
        return errStr;
    }
    // Parse conditions
    QueryParser condParser;
    errStr += condParser.parse(lexer.conditions);
    if(condParser.isError){
        return errStr;
    }
    // Check conditions
    errStr += condParser.check(lexer);
    if(condParser.isError){
        return errStr;
    }
    // Execute
    errStr += executer.execute(lexer,joinParser,condParser);
    if(executer.isError){
        return errStr;
    }
    return errStr + "<h4 style=\"color:green;\">成功執行查詢</h4>";
}

QueryLexer::QueryLexer():isDistinct(false),isError(false),allField(false),allTable(false)
{

}

QString QueryLexer::lex(QString qryStr)
{
    QString errStr = "";
    QString fieldStr = "", tableStr = "", condStr = "", onStr = "";
    int selectIndex, fromIndex, whereIndex, onIndex;
    qryStr = qryStr.trimmed();
    // Check semicolon
    if(!qryStr.endsWith(';')){
        errStr += "<h4 style=\"color:red;\">錯誤：結尾沒有';'字元</h4>";
        isError = true;
    }else {
        qryStr.chop(1);
    }
    // Check 'select'
    if((selectIndex=qryStr.indexOf(QRegularExpression("(^|\\s+)select(\\s+|$)",QRegularExpression::CaseInsensitiveOption),0)) != 0){
        if(selectIndex < 0){
            errStr += "<h4 style=\"color:red;\">錯誤：沒有'select'</h4>";
            isError = true;
        }else{
            errStr += "<h4 style=\"color:red;\">錯誤：'select'前有不合法的值</h4>";
            isError = true;
            qryStr = qryStr.right(qryStr.size() - selectIndex).replace(QRegularExpression("(^|\\s+)select(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        }
    }else{
        qryStr = qryStr.trimmed().replace(QRegularExpression("^select(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        QRegularExpression multi("\\s+select(\\s+|$)",QRegularExpression::CaseInsensitiveOption);
        if(qryStr.indexOf(multi,0) >=0 ){
            errStr += "<h4 style=\"color:red;\">錯誤：不能包含多個'select'</h4>";
            isError = true;
            qryStr = qryStr.replace(multi," ");
        }
    }
    // Check 'from'
    if((fromIndex=qryStr.indexOf(QRegularExpression("(^|\\s+)from(\\s+|$)",QRegularExpression::CaseInsensitiveOption),0)) < 0){
        errStr += "<h4 style=\"color:red;\">錯誤：沒有'from'</h4>";
        isError = true;
    }else{
        fieldStr = qryStr.left(fromIndex);
        qryStr = qryStr.right(qryStr.size() - fromIndex).trimmed().replace(QRegularExpression("^from(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        QRegularExpression multi("\\s+from(\\s+|$)",QRegularExpression::CaseInsensitiveOption);
        if(qryStr.indexOf(multi,0) >=0 ){
            errStr += "<h4 style=\"color:red;\">錯誤：不能包含多個'from'</h4>";
            isError = true;
            qryStr = qryStr.replace(multi," ");
        }
    }
    // Check 'on'
    bool onExist = true;
    if((onIndex=qryStr.indexOf(QRegularExpression("(^|\\s+)on(\\s+|$)",QRegularExpression::CaseInsensitiveOption),0)) < 0){
        tableStr = qryStr;
        onExist = false;
    }else{
        tableStr = qryStr.left(onIndex);
        qryStr = qryStr.right(qryStr.size() - onIndex).trimmed().replace(QRegularExpression("^on(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        QRegularExpression multi("\\s+on(\\s+|$)",QRegularExpression::CaseInsensitiveOption);
        if(qryStr.indexOf(multi,0) >=0 ){
            errStr += "<h4 style=\"color:red;\">錯誤：不能包含多個'on'</h4>";
            isError = true;
            qryStr = qryStr.replace(multi," ");
        }
    }
    // Check 'where'
    if((whereIndex=qryStr.indexOf(QRegularExpression("(^|\\s+)where(\\s+|$)",QRegularExpression::CaseInsensitiveOption),0)) < 0){
        onStr = qryStr;
    }else{
        onStr = qryStr.left(whereIndex);
        if(!onExist){
            tableStr = onStr;
            onStr = "";
        }
        qryStr = qryStr.right(qryStr.size() - whereIndex).trimmed().replace(QRegularExpression("^where(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        QRegularExpression multi("\\s+where(\\s+|$)",QRegularExpression::CaseInsensitiveOption);
        if(qryStr.indexOf(multi,0) >=0 ){
            errStr += "<h4 style=\"color:red;\">錯誤：不能包含多個'where'</h4>";
            isError = true;
            qryStr = qryStr.replace(multi," ");
        }
        condStr = qryStr;
    }
    // Detect distinct
    int distinctIndex;
    if((distinctIndex=fieldStr.indexOf(QRegularExpression("(^|\\s+)distinct(\\s+|$)",QRegularExpression::CaseInsensitiveOption),0)) != 0){
        if(distinctIndex >= 0){
            errStr += "<h4 style=\"color:red;\">錯誤：'distinct'前有不合法的值</h4>";
            isError = true;
            fieldStr = fieldStr.right(fieldStr.size() - distinctIndex).replace(QRegularExpression("(^|\\s+)distinct(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        }
    }else{
        isDistinct = true;
        fieldStr = fieldStr.trimmed().replace(QRegularExpression("^distinct(\\s+|$)",QRegularExpression::CaseInsensitiveOption)," ");
        QRegularExpression multi("\\s+distinct(\\s+|$)",QRegularExpression::CaseInsensitiveOption);
        if(fieldStr.indexOf(multi,0) >=0 ){
            errStr += "<h4 style=\"color:orange;\">警告：除了第一個'distinct'以外的'distinct'會被當作欄位名稱</h4>";
        }
    }
    // Split field
    foreach (QString field, fieldStr.split(',')) {
        field = field.trimmed();
        if(field.isEmpty()){
            errStr += "<h4 style=\"color:red;\">錯誤：欄位名稱不可留白</h4>";
            isError = true;
            break;
        }
        if(field.indexOf(QRegularExpression("\\s|=")) >= 0){
            errStr += "<h4 style=\"color:red;\">錯誤：欄位名稱不可包含空白或'='</h4>";
            isError = true;
            break;
        }
        fields.push_back(field);
    }
    fields.removeDuplicates();
    if(isDistinct && (fields.size() > 1)){
        errStr += "<h4 style=\"color:orange;\">警告：SQL不支援多欄位 'distinct'，只有第一個欄位才會具有唯一值</h4>";
    }
    // Split table
    foreach (QString table, tableStr.split(',')){
        table = table.trimmed();
        if(table.isEmpty()){
            errStr += "<h4 style=\"color:red;\">錯誤：資料表名稱不可留白</h4>";
            isError = true;
            break;
        }
        if(table.indexOf(QRegularExpression("\\s|=")) >= 0){
            errStr += "<h4 style=\"color:red;\">錯誤：資料表名稱不可包含空白或'='</h4>";
            isError = true;
            break;
        }
        tables.push_back(table);
    }
    tables.removeDuplicates();
    // Analyze joins
    onStr = onStr.trimmed();
    QRegularExpression joinRe("^([^=\\s\\(\\)\"']+|=)",QRegularExpression::CaseInsensitiveOption);
    while(1){
        QRegularExpressionMatch match = joinRe.match(onStr);
        if(!match.hasMatch()){
            break;
        }
        QString res = match.captured(0);
        onStr = onStr.mid(res.length()).trimmed();
        joins.push_back(res);
    }
    // Analyze conditions
    condStr = condStr.trimmed();
    QRegularExpression nameRe("^([^=\\s\\(\\)\"']+|\"[^\"]*\"|'[^']*'|=|\\(|\\))",QRegularExpression::CaseInsensitiveOption);
    int countBraces = 0;
    while(1){
        QRegularExpressionMatch match = nameRe.match(condStr);
        if(!match.hasMatch()){
            break;
        }
        QString res = match.captured(0);
        if(res == "("){
            countBraces++;
        }
        if(res == ")"){
            countBraces--;
        }
        condStr = condStr.mid(res.length()).trimmed();
        bool canToDouble = false;
        res.toDouble(&canToDouble);
        if(canToDouble){
            res = "\"" + res + "\"";
        }
        conditions.push_back(res.trimmed());
    }
    if(countBraces){
        errStr += "<h4 style=\"color:red;\">錯誤：'where'存在不成對的括弧</h4>";
        isError = true;
    }
    return errStr;
}

QString QueryLexer::check()
{
    if(manipulators.isEmpty()){
        isError = true;
        return "<h4 style=\"color:red;\">錯誤：沒有選擇資料庫</h4>";
    }
    QString errStr = "";
    // Check table
    foreach (QString table, tables) {
        if(table == "*"){
            allTable = true;
            tables.removeOne(table);
            break;
        }
        if(!manipulators.keys().contains(table)){
            errStr += "<h4 style=\"color:red;\">錯誤：資料表 "+table+" 不存在</h4>";
            tables.removeOne(table);
            isError = true;
            continue;
        }
    }
    if(allTable){
        manipulators.remove("*");
        foreach (QString table, manipulators.keys()) {
            tables.push_back(table);
        }
    }
    foreach (QString field, fields) {
        if(field == "*"){
            allField = true;
            fields.removeOne(field);
            foreach (QString table, tables) {
                fields.append(manipulators[table]->fieldNames);
            }
            break;
        }
        bool exist = false;
        foreach (QString table, tables) {
            if(manipulators[table]->fieldNames.contains(field)){
                exist = true;
                break;
            }
        }
        if(!exist){
            errStr += "<h4 style=\"color:red;\">錯誤：欄位 "+field+" 不存在</h4>";
            fields.removeOne(field);
            isError = true;
        }
    }
    return errStr;
}

void loadManipulator(DBDesc *dbDesc, QString tableName)
{
    if(manipulators.find(tableName) == manipulators.end()){
        manipulators[tableName] = new Manipulator(dbDesc,tableName);
    }
}

QString QueryParser::parse(QStringList &conditions)
{
    isError = false;
    int regCount = 0;
    foreach (QString token, conditions) {
        // Logicals
        if((token.toUpper() == "AND")||(token.toUpper() == "OR")){
            // Generate last logicals
            if(!operators.isEmpty() && (operators.last().type == LOGICAL)){
                ParserNode operation = operators.takeLast();
                if((datas.size() >= 2)&&(datas.at(datas.size()-2).type == RESULT) && (datas.last().type == RESULT)){
                    Instruction newIns;
                    if(operation.value.toUpper() == "OR"){
                        newIns.operation = OR;
                    }else{
                        if(operation.value.toUpper() == "AND"){
                            newIns.operation = AND;
                        }else{
                            isError = true;
                            return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                        }
                    }
                    ParserNode reg1,reg2;
                    reg2 = datas.takeLast();
                    reg1 = datas.last();
                    newIns.regs[0] = reg1;
                    newIns.regs[1] = reg1;
                    newIns.regs[2] = reg2;
                    regCount--;
                    instructions.push_back(newIns);
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                }
            }
            // Insert operation
            ParserNode newOp;
            newOp.type = LOGICAL;
            newOp.value = token;
            operators.push_back(newOp);
            continue;
        }
        // Equal
        if(token == "="){
            // Insert operation
            ParserNode newOp;
            newOp.type = EQUAL;
            newOp.value = token;
            operators.push_back(newOp);
            continue;
        }
        // Bracket
        if(token == "("){
            // Insert operation
            ParserNode newOp;
            newOp.type = BRACKET;
            newOp.value = token;
            operators.push_back(newOp);
            continue;
        }
        if(token == ")"){
            // Generate last logicals
            if(!operators.isEmpty() && (operators.last().type == LOGICAL)){
                ParserNode operation = operators.takeLast();
                if((datas.size() >= 2)&&(datas.at(datas.size()-2).type == RESULT) && (datas.last().type == RESULT)){
                    Instruction newIns;
                    if(operation.value.toUpper() == "OR"){
                        newIns.operation = OR;
                    }else{
                        if(operation.value.toUpper() == "AND"){
                            newIns.operation = AND;
                        }else{
                            isError = true;
                            return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                        }
                    }
                    ParserNode reg1,reg2;
                    reg2 = datas.takeLast();
                    reg1 = datas.last();
                    newIns.regs[0] = reg1;
                    newIns.regs[1] = reg1;
                    newIns.regs[2] = reg2;
                    regCount--;
                    instructions.push_back(newIns);
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                }
            }
            // Remove bracket
            for(int i=operators.size()-1; i>=0; --i){
                if(operators.at(1).type == BRACKET){
                    operators.removeAt(i);
                    break;
                }
            }
            continue;
        }
        // Constant
        if((token.startsWith('"')&&token.endsWith('"'))||(token.startsWith('\'')&&token.endsWith('\''))){
            // Generate last equal
            if(!operators.isEmpty() && (operators.last().type == EQUAL)){
                operators.pop_back();
                if((datas.size() > 0)&& (datas.last().type != RESULT)){
                    Instruction newIns;
                    newIns.operation = EQU;
                    ParserNode data,reg;
                    data = datas.takeLast();
                    reg.type = RESULT;
                    reg.value = QString::number(++regCount);
                    datas.push_back(reg);
                    newIns.regs[0] = reg;
                    newIns.regs[1] = data;
                    newIns.regs[2].type = CONSTENT;
                    token = token.right(token.size()-1);
                    token.chop(1);
                    newIns.regs[2].value = token;
                    instructions.push_back(newIns);
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                }
            }else{
                // Insert constant
                ParserNode reg;
                reg.type = CONSTENT;
                token = token.right(token.size()-1);
                token.chop(1);
                reg.value = token;
                datas.push_back(reg);
            }
            continue;
        }
        // field
        {
            // Insert field
            fields.push_back(token);
            // Generate last equal
            if(!operators.isEmpty() && (operators.last().type == EQUAL)){
                operators.pop_back();
                if((datas.size() > 0) && (datas.last().type != RESULT)){
                    Instruction newIns;
                    newIns.operation = EQU;
                    ParserNode data,reg;
                    data = datas.takeLast();
                    reg.type = RESULT;
                    reg.value = QString::number(++regCount);
                    datas.push_back(reg);
                    newIns.regs[0] = reg;
                    newIns.regs[1] = data;
                    newIns.regs[2].type = FIELD;
                    newIns.regs[2].value = token;
                    instructions.push_back(newIns);
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                }
            }else{
                // Insert field reg
                ParserNode reg;
                reg.type = FIELD;
                reg.value = token;
                datas.push_back(reg);
            }
        }
    }
    // Generate last logicals
    if(!operators.isEmpty() && (operators.last().type == LOGICAL)){
        ParserNode operation = operators.takeLast();
        if((datas.size() >= 2)&&(datas.at(datas.size()-2).type == RESULT) && (datas.last().type == RESULT)){
            Instruction newIns;
            if(operation.value.toUpper() == "OR"){
                newIns.operation = OR;
            }else{
                if(operation.value.toUpper() == "AND"){
                    newIns.operation = AND;
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+operation.value+" 前有不合法的運算子</h4>";
                }
            }
            ParserNode reg1,reg2;
            reg2 = datas.takeLast();
            reg1 = datas.last();
            newIns.regs[0] = reg1;
            newIns.regs[1] = reg1;
            newIns.regs[2] = reg2;
            regCount--;
            instructions.push_back(newIns);
        }else{
            isError = true;
            return "<h4 style=\"color:red;\">錯誤："+operation.value+" 前有不合法的運算子</h4>";
        }
    }
    // Generate return instruction
    Instruction retIns;
    retIns.operation = RET;
    retIns.regs[0] = datas.takeLast();
    instructions.push_back(retIns);
    return "";
}

QString QueryParser::check(QueryLexer &lexer){
    QString errStr = "";
    // Check if field exists
    foreach (QString field, fields) {
        bool exist = false;
        foreach (QString table, lexer.tables) {
            if(manipulators[table]->fieldNames.contains(field)){
                exist = true;
                break;
            }
        }
        if(!exist){
            errStr += "<h4 style=\"color:red;\">錯誤：欄位 "+field+" 不存在</h4>";
            fields.removeOne(field);
            isError = true;
        }
    }
    return errStr;
}

QString QueryExecuter::execute(QueryLexer &lex, JoinParser &joinParser, QueryParser &condParser)
{
    isError = false;
    // Get table list
    if(!joinParser.instructions.isEmpty()){
        tables.push_back(joinParser.instructions.at(0).regs[1].value.split('.').at(0));
        tables.push_back(joinParser.instructions.at(0).regs[2].value.split('.').at(0));
        if(!lex.tables.contains(tables.at(0))){
            return "<h4 style=\"color:red;\">錯誤：資料表 "+tables.at(0)+" 在'from'裡不存在</h4>";
        }
        if(!lex.tables.contains(tables.at(1))){
            return "<h4 style=\"color:red;\">錯誤：資料表 "+tables.at(1)+" 在'from'裡不存在</h4>";
        }
    }else{
        if(lex.tables.size() > 1){
            return "<h4 style=\"color:red;\">錯誤：沒有 'on' 的情況下，'from'裡只能有一個資料表</h4>";
        }else{
            tables = lex.tables;
        }
    }
    // Join tables
    QList<QPair<int,int>*> joinPair;
    if(tables.size() > 1){
        // split table name and field name
        QStringList joinName[2];
        QJsonDocument joinDoc[2];
        joinName[0] = joinParser.fields.at(0).split('.');
        joinDoc[0] = manipulators[joinName[0].at(0)]->fieldDocs[joinName[0].at(1)];
        joinName[1] = joinParser.fields.at(1).split('.');
        joinDoc[1] = manipulators[joinName[1].at(0)]->fieldDocs[joinName[1].at(1)];
        // get field and convert to map
        QMap<QString, QList<int>> joinField[2];
        for(int i=0; i<tables.size(); ++i){
            foreach (QString key, joinDoc[i].object().keys()) {
                QString str = tokenToString(joinName[i].at(0), key);
                QList<int> datas;
                foreach (QJsonValue data, joinDoc[i].object().value(key).toArray()) {
                    datas.push_back(data.toInt());
                }
                joinField[i].insert(str,datas);
            }
        }
        // Generate data pairs
        foreach (QString key, joinField[0].keys()) {
            if(joinField[1].keys().contains(key)){
                foreach (int data1, joinField[0].value(key)) {
                    foreach (int data2, joinField[1].value(key)) {
                        QPair<int,int> *pair = new QPair<int,int>;
                        pair->first = data1;
                        pair->second = data2;
                        joinPair.push_back(pair);
                    }
                }
            }
        }
    }else{
        QJsonObject fieldObj = manipulators[tables.at(0)]->fieldDocs.first().object();
        foreach (QString key, fieldObj.keys()) {
            foreach (QJsonValue val, fieldObj.value(key).toArray()) {
                QPair<int,int> *pair = new QPair<int,int>;
                pair->first = val.toInt();
                pair->second = -1;
                joinPair.push_back(pair);
            }
        }
    }
    // Create field tag
    foreach (QString table, tables) {
        fieldList.append(manipulators[table]->fieldNames);
    }
    bool fieldTag[fieldList.size()];
    for(int i=0; i<fieldList.size(); ++i){
        fieldTag[i] = true;
    }
    // Remove unused field
    QList<QMap<QString,QJsonDocument>> tableDocs;
    foreach (QString table, tables) {
        QMap<QString,QJsonDocument> docs = manipulators[table]->fieldDocs;
        foreach (QString field, docs.keys()) {
            if(!lex.fields.contains(field)){
                fieldTag[fieldList.indexOf(QRegularExpression(field))] = false;
            }
            if(!lex.fields.contains(field) && !condParser.fields.contains(field)){
                docs.remove(field);
            }
        }
        tableDocs.push_back(docs);
    }
    for(int i=0,j=0; i<fieldList.size();++j){
        if(fieldTag[j]){
            ++i;
        }else {
            fieldList.removeAt(i);
        }
    }
    // Run Instruction
    QList<QList<QList<int>*>*> regs;
    QList<QList<int>*> *res;
    foreach (Instruction instruction, condParser.instructions) {
        if(instruction.operation == EQU){
            // Create new register
            QList<QList<int>*> *newReg = new QList<QList<int>*>;
            newReg->push_back(new QList<int>);
            newReg->push_back(new QList<int>);
            if(instruction.regs[1].type == FIELD){
                // get field 1
                QJsonObject field1;
                int index1 = 0;
                for(int i=0; i<2; ++i) {
                    if(tableDocs[i].keys().contains(instruction.regs[1].value)){
                        field1 = tableDocs[i][instruction.regs[1].value].object();
                        index1 = i;
                        break;
                    }
                }
                bool existValue = false;
                if(instruction.regs[2].type == FIELD){
                    // get field 2
                    QJsonObject field2;
                    int index2 = 0;
                    for(int i=0; i<2; ++i) {
                        if(tableDocs[i].keys().contains(instruction.regs[2].value)){
                            field2 = tableDocs[i][instruction.regs[2].value].object();
                            index2 = i;
                            break;
                        }
                    }
                    // Compare
                    if(index1 == index2){
                        foreach (QString key, field1.keys()) {
                            if(field2.keys().contains(key)){
                                existValue = true;
                                QJsonArray arr[2];
                                arr[0] = field1.value(key).toArray();
                                arr[1] = field2.value(key).toArray();
                                foreach (QJsonValue val, arr[0]) {
                                    if(arr[1].contains(val)){
                                        newReg->at(index1)->push_back(val.toInt());
                                    }
                                }
                            }
                        }

                    }else{
                        foreach (QString key, field1.keys()) {
                            QString uToken = stringToToken(tables.at(index2),tokenToString(tables.at(index1),key));
                            if(!uToken.isEmpty()){
                                existValue = true;
                                foreach (QJsonValue val, field1.value(key).toArray()) {
                                    newReg->at(index1)->push_back(val.toInt());
                                }
                                foreach (QJsonValue val, field2.value(uToken).toArray()) {
                                    newReg->at(index2)->push_back(val.toInt());
                                }
                            }
                        }
                    }
                }else{
                    QString tok = stringToToken(tables[index1],instruction.regs[2].value);
                    if(field1.keys().contains(tok)){
                        existValue = true;
                        foreach (QJsonValue val, field1.value(tok).toArray()) {
                            newReg->at(index1)->push_back(val.toInt());
                        }
                    }
                }
                if(!existValue){
                    delete newReg->at(index1);
                    newReg->replace(index1,NULL);
                }
            }else{
                if(instruction.regs[2].type == FIELD){
                    // get field 2
                    QJsonObject field2;
                    int index2 = 0;
                    bool existValue = false;
                    for(int i=0; i<2; ++i) {
                        if(tableDocs[i].keys().contains(instruction.regs[2].value)){
                            field2 = tableDocs[i][instruction.regs[2].value].object();
                            index2 = i;
                            break;
                        }
                    }
                    // compare
                    QString tok = stringToToken(tables[index2],instruction.regs[1].value);
                    if(field2.keys().contains(tok)){
                        existValue = true;
                        foreach (QJsonValue val, field2.value(tok).toArray()) {
                            newReg->at(index2)->push_back(val.toInt());
                        }
                    }
                    if(!existValue){
                        delete newReg->at(index2);
                        newReg->replace(index2,NULL);
                    }
                }else{
                    if(instruction.regs[1].value == instruction.regs[2].value){
                        // All true
                        QJsonObject field[2];
                        field[0] = tableDocs[0].first().object();
                        foreach (QString key, field[0].keys()) {
                            foreach (QJsonValue val, field[0].value(key).toArray()) {
                                newReg->at(0)->push_back(val.toInt());
                            }
                        }
                        field[1] = tableDocs[1].first().object();
                        foreach (QString key, field[1].keys()) {
                            foreach (QJsonValue val, field[1].value(key).toArray()) {
                                newReg->at(1)->push_back(val.toInt());
                            }
                        }
                    }else{
                        // All false
                        delete newReg->at(0);
                        newReg->replace(0,NULL);
                        delete newReg->at(1);
                        newReg->replace(1,NULL);
                    }
                }
            }
            regs.push_back(newReg);
        }
        if(instruction.operation == AND){
            QList<QList<int>*> *reg1,*reg2;
            reg2 = regs.takeLast();
            reg1 = regs.last();
            // compare
            for(int i=0; i<2; ++i){
                if(reg1->at(i) == NULL){
                    if(reg2->at(i) != NULL){
                        delete reg2->at(i);
                        reg2->replace(i,NULL);
                    }
                }else{
                    if(reg1->at(i)->isEmpty()){
                        delete reg1->at(i);
                        reg1->replace(i,new QList<int>(*(reg2->at(i))));
                    }else{
                        if(reg2->at(i) == NULL){
                            delete reg1->at(i);
                            reg1->replace(i,NULL);
                        }else{
                            if(!reg2->at(i)->isEmpty()){
                                foreach (int data, *(reg1->at(i))) {
                                    if(!reg2->at(i)->contains(data)){
                                        reg1->at(i)->removeOne(data);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // clean reg2
            for(int i=0; i<2; ++i){
                delete reg2->at(i);
            }
            delete reg2;
        }
        if(instruction.operation == OR){
            QList<QList<int>*> *reg1,*reg2;
            reg2 = regs.takeLast();
            reg1 = regs.last();
            // compare
            for(int i=0; i<2; ++i){
                if(reg1->at(i) == NULL){
                    if(reg2->at(i) != NULL){
                        reg1->replace(i,new QList<int>(*(reg2->at(i))));
                    }
                }else{
                    if(!reg1->at(i)->isEmpty()){
                        if(reg2->at(i) != NULL){
                            if(!reg2->at(i)->isEmpty()){
                                foreach (int data, *(reg2->at(i))) {
                                    if(!reg1->at(i)->contains(data)){
                                        reg1->at(i)->push_back(data);
                                    }
                                }
                            }else{
                                reg1->at(i)->clear();
                            }
                        }
                    }
                }
            }
            // clean reg2
            for(int i=0; i<2; ++i){
                delete reg2->at(i);
            }
            delete reg2;
        }
        if(instruction.operation == RET){
            res = regs.at(instruction.regs[0].value.toInt()-1);
        }
    }
    // Remove unqualified data
    for(int i=0; i<joinPair.size();){
        QPair<int,int> *pair = joinPair.at(i);
        if((res->at(0)==NULL)||((!res->at(0)->isEmpty())&&(!res->at(0)->contains(pair->first)))){
            delete pair;
            joinPair.removeAt(i);
        }else{
            if(tables.size() > 1){
                if((res->at(1)==NULL)||((!res->at(1)->isEmpty())&&(!res->at(1)->contains(pair->second)))){
                    delete pair;
                    joinPair.removeAt(i);
                }else{
                     ++i;
                }
            }else{
                ++i;
            }
        }
    }
    // Clean regs
    for(int i=0; i<regs.size(); ++i){
        for(int j=0; j<2; ++j){
            delete regs.at(i)->at(j);
        }
        delete regs.at(i);
    }
    // Fetch and merge data
    for(int i=0, tagIndex=0; i<tables.size(); ++i){
        QFile &dataFile = manipulators[tables.at(i)]->dataFile;
        dataFile.open(QFile::ReadOnly);
        int fieldSize = manipulators[tables.at(i)]->fieldNames.size();
        QDataStream stream(&dataFile);
        for(int j=0; j<joinPair.size(); ++j){
            if(datas.size() <= j){
                datas.push_back(QStringList());
            }
            QPair<int,int> *pair = joinPair.at(j);
            // Load
            if(i == 0){
                dataFile.seek(pair->first * (fieldSize * 2 + 1) * sizeof(int));
            }else{
                dataFile.seek(pair->second * (fieldSize * 2 + 1) * sizeof(int));
            }
            int dirty;
            stream >> dirty;
            for(int k=0; k<fieldSize; ++k){
                int hash, offset;
                stream >> hash;
                stream >> offset;
                if(fieldTag[tagIndex+k]){
                    datas[j].push_back(tokenToString(tables.at(i),QString::number(hash)+"_"+QString::number(offset)));
                }
            }
        }
        dataFile.close();
        tagIndex = fieldSize;
    }
    return "";
}

QString tokenToString(QString table, QString token)
{
    QString ret = "";
    int hash = token.split('_').at(0).toInt();
    int offset = token.split('_').at(1).toInt();
    QFile *hashfile = manipulators[table]->hashFiles.at(hash);
    hashfile->open(QFile::ReadOnly);
    hashfile->seek(offset);
    QDataStream stream(hashfile);
    int dirty,len;
    stream >> dirty;
    stream >> len;
    QChar storedToken[len/2+1];
    storedToken[len/2] = '\0';
    stream.readRawData((char *)storedToken,len);
    ret = QString(storedToken);
    hashfile->close();
    return ret;
}

QString JoinParser::parse(QStringList &joins)
{
    isError = false;
    int regCount = 0;
    foreach (QString token, joins) {
        // Equal
        if(token == "="){
            // Insert operation
            ParserNode newOp;
            newOp.type = EQUAL;
            newOp.value = token;
            operators.push_back(newOp);
            continue;
        }
        // field
        {
            // Generate last equal
            if(!operators.isEmpty() && (operators.last().type == EQUAL)){
                operators.pop_back();
                if((datas.size() > 0) && (datas.last().type != RESULT)){
                    Instruction newIns;
                    newIns.operation = EQU;
                    ParserNode data,reg;
                    data = datas.takeLast();
                    reg.type = RESULT;
                    reg.value = QString::number(++regCount);
                    datas.push_back(reg);
                    newIns.regs[0] = reg;
                    newIns.regs[1] = data;
                    newIns.regs[2].type = FIELD;
                    newIns.regs[2].value = token;
                    if(instructions.isEmpty()){
                        instructions.push_back(newIns);
                    }else{
                        return "<h4 style=\"color:orange;\">警告：SQL不支援多欄位 join 操作，只有第一個 'on' 條件會被執行</h4>";
                    }
                }else{
                    isError = true;
                    return "<h4 style=\"color:red;\">錯誤："+token+" 前有不合法的運算子</h4>";
                }
            }else{
                // Insert field reg
                ParserNode reg;
                reg.type = FIELD;
                reg.value = token;
                datas.push_back(reg);
                if(!instructions.isEmpty()){
                    return "<h4 style=\"color:orange;\">警告：SQL不支援多欄位 join 操作，只有第一個 'on' 條件會被執行</h4>";
                }
            }
            // Insert field
            fields.push_back(token);
        }
    }
    return "";
}

QString JoinParser::check(QueryLexer &)
{
    QString errStr = "";
    // Check if field exists
    foreach (QString field, fields) {
        QString table = field.split('.').at(0);
        if(!manipulators.keys().contains(table)){
            errStr += "<h4 style=\"color:red;\">錯誤："+table+"資料表不存在</h4>";
            isError = true;
            continue;
        }
        if(!manipulators[table]->fieldNames.contains(field.split('.').at(1))){
            errStr += "<h4 style=\"color:red;\">錯誤："+table+"資料表內不存在"+field.split('.').at(1)+"欄位</h4>";
            fields.removeOne(field);
            isError = true;
        }
    }
    return errStr;
}

QString stringToToken(QString table, QString str)
{
    // Get hash
    int retHash,retOffset = -1;
    retHash = hash33(str, manipulators[table]->hashFiles.count());
    // Find token
    QFile *hashFile =manipulators[table]->hashFiles.at(retHash);
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
        if(str == QString(storedToken)){
            retOffset = hashFile->pos() - len - 2*sizeof(int);
            break;
        }
    }
    hashFile->close();
    if(retOffset == -1){
        return "";
    }else{
        return QString::number(retHash) + "_" + QString::number(retOffset);
    }
}
