#ifndef DBWRAP_H
#define DBWRAP_H
/************************************************************************
* dbWrap
* 1、封装数据库的的打开关闭接口。
* 2、封装数据库的数据查询修改接口。（含读写互斥锁）
* 3、封装数据库拷贝接口。（运行时建立内存数据库）
* 4、含有一个qSqlDatabase指针
* Author:linbin
* Email:13918814219@163.com
************************************************************************/
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <typeinfo>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>
#include <QFileInfo>

class DBWrap
{
public:
    explicit DBWrap(const QString strType);
    virtual ~DBWrap();

    bool openDB(const QString strSvrName,
                const QString strDBname,
                const QString strUserID = "",
                const QString strUserPwd = "");

    bool openDB(const QString strDBname);

    /*
     * close db
     */
    void closeDB();
    /*
     *excute a SQL, without return query. For writing data or configure db
     */
    bool excuteSQL(const QString);
    /*
     * excute a SQL with return query. For reading data.
     * Noticed! return value could be nullptr
     * Noticed! return value need to be deleted, using closeRecordset()
     */
    void *openRecordsetBySql(const QString strSql);
    /*
     * delete query. For reading data.
     */
    void closeRecordset(void *);
    /*
     * is open?
     */
    bool isOpen() const;
    /*
     * is it end of the query. For reading data.
     */
    bool recEOF(void *p) const;
    /*
     * is it begin of the query. For reading data.
     */
    bool recBOF(void *p) const;
    /*
     * uncompleted!!! only for SQLITE.
     */
    bool isTableExist(const QString strTable);
    /*
     *delete table
     */
    bool dropTable(const QString);
    /*
     * get record count of the query. For reading data.
     * return -1: query not exist or database not opened
     */
    long getRecordCount(void *p) const;
    /*
     * move to the first record of the query. For reading data.
     */
    bool moveFirst(void *p) const;
    /*
     * move previous record of the query. For reading data.
     */
    bool movePrevious(void *p) const;
    /*
     * move next record of the query. For reading data.
     */
    bool moveNext(void *p) const;
    /*
     * move last record of the query. For reading data.
     */
    bool moveLast(void *p) const;
    /*
     * move to a certain record of the query. For reading data.
     */
    bool moveTo(int n, void *p) const;

    //transaction not exist, return false
    bool transaction();
    //transaction not exist, return false
    bool commit();
    //transaction not exist, return false
    bool rollback();

    QString getDbName() const
    {
        return __strDbName;
    }

    /*
     * sqliteDBMemFile 针对sqlite3内存型数据库的 save和load封装
     * just for sqlite memory database. dump to file or load from file
     * 输入参数：
     * 参数1：QString filename 文件名称
     * 参数2：bool save true保存到文件，false从文件读取
     * 返回数值：
     * 1、成功true
     * 2、失败false （不是sqlite数据库，filename不成立）
     * 功能描述：
     * 1、数据库保存和读取
     */
    bool sqliteDBMemFile(QString filename, bool save );


protected:
    bool __openMDB(const QString strMDBname, QString strUserID = "sa", const QString strPassword = "");
    bool __openMDBByUDL(const QString strUDL);
    bool __connectSqlServer(const QString strSvrName, const QString strDBname, const QString strUserID, const QString strUserPwd);
    bool __openSqlite(const QString &strDbName);
    bool __openMySql(QString strSvrName, const QString strDBname, QString strUserID, const QString strUserPwd);

private:

    QString __strConnName;
    QString __strDbType;
    QString __strDbName;
    QList<QSqlQuery *> __lstQrys;

    QSqlDatabase *m_pDB;
public:
    /*
     * readFieldsValueFromRec
     * 从某个QSqlQuery中的某条记录，读取字段和相应的值
     * 输入参数：
     * 参数1：QSqlQuery*查询结果
     * 参数2：str 第一个字段名
     * 参数3：t   第一个字段名对应值
     * 参数4：arg 第二个字段名       （可扩展）
     * 参数5：arg 第二个字段名对应值  （可扩展）
     * ………
     * 返回数值：
     * 1、成功true
     * 2、失败false （QSqlQuery不正确）
     * 功能描述：
     * 1、连续读取值
     */
    template <typename T, typename ...Args>
    //variadic template （可变参数模板）
    bool readFieldsValueFromRec(void *p, const QString &str, T &t, Args &... args)
    {
        const QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
        if(nullptr == pQry || false == pQry->isValid())
        {
            return false;
        }
        //get value of str from pQry, assign to t
        //value()
        //An invalid QVariant is returned if field index does not exist,
        //if the query is inactive, or if the query is positioned on an invalid record.
        //if invalid QVariant is returned, t will not be touched.
        assignVal(pQry->value(str), t);
        QString strFldName;
        int arr[] = {(extractArg(pQry, strFldName, args), 0)...};
        return true;
    }
    /*
     * addFieldsValueToTbl
     * insert fields and values to a table ,create a new record.
     * 输入参数：
     * 参数1：QString &strTbl 表名称
     * 参数2：QString &strFld 第一个字段名
     * 参数3：T &t            第一个字段名对应值
     * 参数4：Args & args     第二个字段名（可扩展）
     * 参数5：Args & args     第二个字段名对应值（可扩展）
     * ………
     * 返回数值：
     * QSqlQuery.exec()返回值
     * 功能描述：
     * 1、数据库新增记录
     */
    template<typename T, typename ... Args>
    bool addFieldsValueToTbl(const QString &strTbl, const QString &strFld, const T &t, const Args &... args)
    {
        QStringList lstFlds;
        QList<QVariant> lstVars;
        lstFlds << strFld;
        lstVars.push_back(t);//equals to .append(t)
        int arr[] = {(extractArg1(lstFlds, lstVars, args), 0)...};
        QString strFlds;
        QString strPlaceholders;
        for(const QString &str : lstFlds)
        {
            strFlds += "," + str;
            strPlaceholders += ",:" + str;//repalce holders for value
        }
        QString strSql = QString("INSERT INTO %1(%2) VALUES (%3)").arg(strTbl).arg(strFlds.mid(1)).arg(strPlaceholders.mid(1));
        QSqlQuery qry(*m_pDB);
        qry.prepare(strSql);
        const int iCount = lstFlds.size();
        for(int i = 0; i < iCount; ++i)
        {
            qry.bindValue(":" + lstFlds.at(i), lstVars.at(i));
        }
        const bool bRet = qry.exec();
        //QSqlError lastError = qry.lastError();
        //QString strErr = lastError.driverText();
        return bRet;
    }
    /*
     * updateTblFieldsValue
     * update fields and values to a table ,don‘t create any new records.
     * 输入参数：
     * 参数1：QString &strTbl   表名称
     * 参数2：QString &strWhere 条件筛选（Where）语句
     * 参数3：QString &strFld   第一个字段名
     * 参数4：T &t              第一个字段名对应值
     * 参数5：Args & args       第二个字段名（可扩展）
     * 参数6：Args & args       第二个字段名对应值（可扩展）
     * ………
     * 返回数值：
     * QSqlQuery.exec()返回值
     * 功能描述：
     * 1、数据库更新记录
     */
    template<typename T, typename ... Args>
    bool updateTblFieldsValue(const QString &strTbl, const QString &strWhere, const QString &strFld, const T &t, const Args &... args)
    {
        QStringList lstFlds;
        QList<QVariant> lstVars;
        lstFlds << strFld;
        lstVars << t;
        int arr[] = {(extractArg1(lstFlds, lstVars, args), 0)...};
        //---------------------------
        QString strFlds;
        for(const QString &str : lstFlds)
        {
            strFlds += "," + str + "=:" + str;
        }
        QString strSql = QString("UPDATE %1 SET %2 %3").arg(strTbl).arg(strFlds.mid(1)).arg(strWhere);
        QSqlQuery qry(*m_pDB);
        qry.prepare(strSql);
        //qDebug() << strSql;
        const int iCount = lstFlds.size();
        for(int i = 0; i < iCount; ++i)
        {
            qry.bindValue(":" + lstFlds.at(i), lstVars.at(i));
        }
        const bool bRet = qry.exec();
        //QSqlError lastError = qry.lastError();
        //QString strErr = lastError.driverText();
        return bRet;
    }

private:
    //assign value, from QVariant var to T t
    template<typename T>
    void assignVal(const QVariant && var, T &t) //C++2011标准的右值引用语法,当传入临时对象时可以避免一次拷贝 (const QVariant && var)
    {
        const static size_t __intID = typeid (int).hash_code();
        const static size_t __uintID = typeid (unsigned int).hash_code();
        const static size_t __boolID = typeid (bool).hash_code();
        const static size_t __doubleID = typeid (double).hash_code();
        const static size_t __floatID = typeid (float).hash_code();
        const static size_t __llID = typeid (long long).hash_code();
        const static size_t __ullID = typeid (unsigned long long).hash_code();
        const static size_t __QStringID = typeid (QString).hash_code();
        const static size_t __QDateTimeID = typeid (QDateTime).hash_code();
        const static size_t __QDateID = typeid (QDate).hash_code();
        const static size_t __QTimeID = typeid (QTime).hash_code();
        const static size_t __QByteArrayID = typeid (QByteArray).hash_code();

        if(false == var.isValid())
            return;

        const size_t id = (typeid (t).hash_code());
        T *p = &t;
        if(id == __intID)
        {
            *(int *)p = var.toInt();
        }
        else if(id == __uintID)
        {
            *(unsigned int *)p = var.toUInt();
        }
        else if(id == __boolID)
        {
            *(bool *)p = var.toBool();
        }
        else if(id == __doubleID)
        {
            *(double *)p = var.toDouble();
        }
        else if(id == __floatID)
        {
            *(float *)p = var.toFloat();
        }
        else if(id == __llID)
        {
            *(long long *)p = var.toLongLong();
        }
        else if(id == __ullID)
        {
            *(unsigned long long *)p = var.toULongLong();
        }
        else if(id == __QStringID)
        {
            *(QString *)p = var.toString();
        }
        else if(id == __QDateTimeID)
        {
            *(QDateTime *)p = var.toDateTime();
        }
        else if(id == __QDateID)
        {
            *(QDate *)p = var.toDate();
        }
        else if(id == __QTimeID)
        {
            *(QTime *)p = var.toTime();
        }
        else if(id == __QByteArrayID)
        {
            *(QByteArray *)p = var.toByteArray();
        }
    }
    //get the value in a certain field from a certain Query
    template <typename T>
    void extractArg(const QSqlQuery *pQry, QString & strFldName, T &t)
    {
        if(strFldName.isEmpty())
        {
            //t to strFldName
            strFldName = QVariant(t).toString();
        }
        else
        {
            //value()
            //An invalid QVariant is returned if field index does not exist,
            //if the query is inactive, or if the query is positioned on an invalid record.
            //if invalid QVariant is returned, t will not be touched.
            assignVal(pQry->value(strFldName), t);
            strFldName.clear();
        }
    }
    //add fields & values to table. T is both fieled name and value, depending on the consquence.
    template <typename T>
    void extractArg1(QStringList &lstFlds, QList<QVariant> &lstVars, const T &t)
    {
        const QVariant var(t);
        if(lstFlds.size() == lstVars.size())
        {
            lstFlds << var.toString();
        }
        else
        {
            lstVars << var;
        }
    }

};

#endif // DBWRAP_H
