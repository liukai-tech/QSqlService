#include "SqlService.h"

SqlService::SqlService()
{
    
}

/**
  * @brief  open 
  * @note   打开指定文件数据库，没有则创建
  * @param  name(数据库文件)， type(打开方式)
  * @retval 返回是否打开成功．
  */
bool SqlService::open( QString name, const QString& type )
{
    if(QSqlDatabase = QSqlDatabase::database(name)) {
        m_sqlDatabase = QSqlDatabase::database(name);
    }else{
        /* 添加数据库驱动 */
        m_sqlDatabase = QSqlDatabase::addDatabase(type, name);
        /* 数据库连接命名 */
        m_sqlDatabase.setDatabaseName(name);
    }

    /* 打开数据库 */
    if(!m_sqlDatabase.open()) {
        setLastError(m_sqlDatabase.lastError().text());
        return false;
    }

    /* 以下执行相关SQL语句 */
    m_sqlQuery = QSqlQuery(m_sqlDatabase);
    return true;
}

/**
  * @brief  createTable 
  * @note   从已经打开的数据库创建一个指定字段的表格
  * @param  table(数据库的某一个表格)，map(表格的字段或列字段(key)和类型(value))
  * @retval 返回是否创建成功
  */
bool SqlService::createTable(QString table, QMap<QString, QString> map)
{
    if(!isTableExist(table)) {
        QString tableList = QString("create table %1 (").arg(table);
        QMapIterator<QString, QString> i(map);
        while (i.hasNext()) {
            i.next();
            tableList.append(QString("%1 %2")).arg(i.key()).arg(i.value());
            if (i.hasNext())
                tableList.append(", ");
        }
        tableList.append(")");
        return this->exec(tableList);
    }else{
        setLastError(QString("Exist table<%1>").arg(table));
    }   
}

/**
  * @brief  insertRowTable 
  * @note   向表格插入一行
  * @param  table(数据库的某一表格)，map(数据的字段或列(key)，值(value))
  * @retval 返回是否插入成功
  */
bool SqlService::insertRowTable(QString table, QVariantMap map)
{
    QMap<QString, QString> tableContentMap;

    if(!isTableExist(table)) {
        setLastError(QString("Not find %1 table!").arg(table));
        return false;
    }else {
        tableContentMap = getTableInfo(table);
        if (tableContentMap.isEmpty())
            return false;
    }

    QString insertTableContent = QString("insert into %1 (").arg(table);
    QString values = QString("values (");

    QMapIterator<QString, QString> i(tableContentMap);
    while (i.hasNext()) {
        i.next();
        insertTableContent.append(QString("%1").arg(i.key()));
        values.append("?");
        if (i.hasNext()) {
            insertTableContent.append(", ");
            values.append(", ");
        }
    }
    insertTableContent.append(") ");
    values.append(")");

    insertTableContent.append(values);

    if (!this->prepare(insertTableContent)) {
        return false;
    }

    QMapIterator<QString, QString> ii(tableContentMap);
    while (ii.hasNext()) {
        ii.next();
        m_sqlQuery.addBindValue(map[ii.key()]);
    }

    return this->exec();
 }

/**
  * @brief  insertColumnTable 
  * @note   向表格插入一列
  * @param  table(数据库的某一表格)，name(列字段)，type(列的类型)
  * @retval 返回是否插入成功
  */
/* ALTER TABLE table_name ADD column_name datatype */
bool SqlService::insertColumnTable(QString table, QString name, QString type)
{
     QString content = QString("alter table %1 add %2 %3").arg(table).arg(name).arg(type);
     return this->exec(content);
}

/**
  * @brief  updateRowTable 
  * @note   更新表格的一行内容，替换出name对应的值，其余的都替换
  * @param  table(数据库的某一表格)，name(列字段的值)，map(更新的内容)
  * @retval 返回是否更新成功
  */
/* UPDATE 表名称 SET 列名称 = 新值 WHERE 列名称 = 某值 */
bool SqlService::updateRowTable(QString table, QString name, QVariantMap map)
{
    QString content = QString("update %1 set ").arg(table);

    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();

        if (i.hasNext())
            content += QString("%1 = '%2', ").arg(i.key()).arg(i.value().toString());
        else
            content += QString("%1 = '%2' ").arg(i.key()).arg(i.value().toString());      
    }

    content += QString("where %1 = %2").arg(name).arg(map[name].toString());

    return this->exec(content);
}

/**
  * @brief  updateRowTable 
  * @note   更新表格的一行内容，替换map的所有内容
  * @param  table(数据库的某一表格)，targetKey(列字段)，targetValue(列字段对应的值)，map(更新的内容)
  * @retval 返回是否更新成功
  */
bool SqlService::updateRowTable(QString table, QString targetKey, QString targetValue, QVariantMap map)
{
    QString content = QString("update %1 set ").arg(table);

     QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();

        if (i.hasNext())
            content += QString("%1 = '%2', ").arg(i.key()).arg(i.value().toString());
        else
            content += QString("%1 = '%2' ").arg(i.key()).arg(i.value().toString());      
    }

    content += QString("where %1 = %2").arg(targetKey).arg(targetValue);

    return this->exec(content);  
}

/**
  * @brief  deleteRowTable 
  * @note   删除表格的一行
  * @param  table(数据库的某一表格)，columnName(列字段)，value(列字段的值)
  * @retval 返回是否删除成功
  */
bool SqlService::deleteRowTable(QString table, QString columnName, QString value)
{
    QString deleteContent = QString("delete from %1 where %2 = %3").arg(table).arg(columnName).arg(value);
    return this->exec(deleteContent);
}

/**
  * @brief  deleteRowTable 
  * @note   删除表格的一行
  * @param  table(数据库的某一表格)，columnName(列字段)
  * @retval 返回是否删除成功
  */
bool SqlService::deleteColumnTable(QString table, QString columnName)
{
    /* ALTER TABLE table_name DROP COLUMN column_name
     * QSQLITE不支持
     */ 
    if (m_sqlDatabase.driverName() != "QSQLITE") {
        QString content = QString("alter table %1 drop column %2").arg(table).arg(columnName);
        return this->exec(content);
    }else{
        return false;
    }
    
}

/**
  * @brief  sortTable 
  * @note   按升序排序表格
  * @param  table(数据库的某一表格)，target(列字段)
  * @retval 返回是否排序成功
  */
bool SqlService::sortTable(QString table, QString target)
{
    /* select * from table order by target */
    QString sortContent = QString("select * from % order by %2").arg(table).arg(target);
    return this->exec(sortContent);
}

/**
  * @brief  sortTable 
  * @note   返回数据库的搜索结果，注意一般调用该函数是执行了一些数据操作，比如排序，查询数据后．另外调用size函数会重置搜索结果为first
  * @param  无参数
  * @retval 返回搜索所有结果
  */
int SqlService::size()
{
    int size = -1;
    while (m_sqlQuery.next()) {
        /* 驱动支持返回记录数 */
        if (m_sqlQuery.driver()->hasFeature(QSqlDriver::QuerySize)) {
            size = m_sqlQuery.size();
            break;
        }else { /* 驱动不支持返回记录数，只能循环查找 */
            m_sqlQuery.last();
            size = m_sqlQuery.at() + 1;
        }
    }

    m_sqlQuery.first();
    return size;
}

/**
  * @brief  lastError 
  * @note   返回错误代码
  * @param  无参数
  * @retval 返回错误代码的值
  */
QString SqlService::lastError()
{
    return m_lastError;
}

/**
  * @brief  getSqlQuery 
  * @note   返回数据库相关类，方便上层有需要的操作
  * @param  无参数
  * @retval 返回数据库相关类
  */
QSqlQuery& SqlService::getSqlQuery()
{
    return m_sqlQuery;
}

/**
  * @brief  getSqlDatabase 
  * @note   返回数据库相关类，方便上层有需要的操作
  * @param  无参数
  * @retval 返回数据库相关类
  */
QSqlDatabase& SqlService::getSqlDatabase()
{
    return m_sqlDatabase;
}

/**
  * @brief  isTableExist 
  * @note   判断表格是否存在
  * @param  数据库的某一表格
  * @retval 返回判断结果
  */
bool SqlService::isTableExist(QString table)
{
    return m_sqlDatabase.tables().contains(table);
}

/**
  * @brief  prepare 
  * @note   QSqlQuery prepare的封装
  * @param  解析的数据
  * @retval 是否操作成功
  */
bool SqlService::prepare(const QString &query)
{
    if (!m_sqlQuery.prepare(query)) {
        setLaetError(m_sqlQuery.lastError().text());
        return false;
    }else {
        return true;
    }
}

/**
  * @brief  exec 
  * @note   对QSqlQuery exec的封装
  * @param  解析的数据
  * @retval 是否操作成功
  */
bool SqlService::exec(const QString &query)
{
    if (!m_sqlQuery.exec(QString(query))) {
        setLastError(m_sqlQuery.lastError().text());
        return false;
    }else {
        return true;
    }
}

/**
  * @brief  exec 
  * @note   对QSqlQuery exec的封装
  * @param  无参数
  * @retval 是否操作成功
  */
bool SqlService::exec()
{
    if (!m_sqlQuery.exec()) {
        setLastError(m_sqlQuery.lastError().text());
        return false;
    }else {
        return true;
    }
}

/**
  * @brief  getTableInfo 
  * @note   获取表格的所有列字段
  * @param  数据库的某一表格
  * @retval 返回获取结果QMap<QString, QString>
  */
QMap<QString, QString> SqlService::getTableInfo(QString table)
{
    QMap<QString, QString> tableMap;

    QString str = "PRAGMA table_info(" + table + ")";
    m_sqlQurey.perpare(str);

    if (m_sqlQuery.exec()){
        while (m_sqlQuery.next()){
            /* value(0) = 字段名； value(1) = 字段类型 */
            tableMap[m_sqlQuery.value(1).toString()] = m_sqlQuery.value(2).toString();
        }
        return tableMap;
    }else{
        setLastError(m_sqlQuery.lastError().text());
        return tableMap;
    }
}

/**
  * @brief  setLastError 
  * @note   设置错误代码
  * @param  错误代码
  * @retval 无返回
  */
void SqlService::setLastError(QString lastError)
{
    m_lastError = lastError;
    qDebug() << m_lastError;
}

/**
  * @brief  getValues
  * @note   获取指定的数据库数据(注意：不建议一次获取过多数目)
  * @param  page(获取多少页)，pageNum(每一页获取多少条)
  * @retval 返回获取结果QList<QVariantMap>
  */
QList<QVariantMap> SqlService::getValues(int page, int pageNum)
{
    QList<QVariantMap> list;
    if (!m_sqlQuery.seek(page)) {
        setLastError("getValues error![The number of pages is beyond the limit]");
        return list;
    }

    do {
        QVariantMap map;
        for (int i = 0; i < m_sqlQuery.record().count(); i++) {
            map.insert(m_sqlQuery.record().field(i).name(), m_sqlQuery.record().field(i).value());
        }
        list.append(map);

    }while(m_sqlQuery.next() && --pageNum);

    return list;
}

