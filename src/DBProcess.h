#ifndef __INC_CDBPROCESS_H__82075B33_
#define __INC_CDBPROCESS_H__82075B33_

/************************************************************************
*Description:                                                           *
*Author: realfan                                                        *
*Date of Created:                                                       *
*Date of Modified 2016.12.07 Purpose of Modify Person Name of Modify    *
*------------------  ----------------------  -------------------------  *
*                                                                       *
************************************************************************/
//#include "stdarg.h"
#include <QString>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QSqlQuery>
#include <typeinfo>
class QSqlDatabase;
class QSqlQuery;

class CDBProcess
{
public:
    //init with database type
    CDBProcess(const QString strType = "sqlite"); //sqlite  mysql  access sqlserver
    virtual ~CDBProcess();
    //remote database
    bool openDB(const QString strSvrName,
                const QString strDBname,
                const QString strUserID = "",
                const QString strUserPwd = "");

    //local database
    bool openDB(const QString strDBname);
    void closeDB();

    //return whether processed successful
    bool excuteSQL(const QString);

    //return the qurey result: QSqlQuery* as void*
    void *openRecordsetBySql(const QString strSql);
    //remove the qurey result inside the list
    void closeRecordset(void *);

    bool isOpen() const;
    //is the query p's position at end of file?
    bool recEOF(void *p) const;
    //is the query p's position at begin of file?
    bool recBOF(void *p) const;
    bool isTableExist(const QString strTable);
    bool dropTable(const QString);
    //QStringList GetTableNames();
    //========读取==================================
    //return -1: query not exist or database not opened
    long getRecordCount(void *p) const;
    bool moveFirst(void *p) const;
    bool movePrevious(void *p) const;
    bool moveNext(void *p) const;
    bool moveLast(void *p) const;
    bool moveTo(int n, void *p) const;

    //bool execStoreProcOfArgList(int idx, const QString strStoreProc, const char *szFldsInfo, ...);
    //bool exexProc(const QString strStoreProc, QString str1, QString &str2);

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
public:
    //==============================================
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
public:
    //read data in certain fieldName from table's current record.
    //arg0: filedName1, arg1: data1, arg2: filedName2, arg3: data2 ... ...
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
    //---------------------------
private:
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
public:
    //insert fields and values to a table ,create a new record.
    template<typename T, typename ... Args>
    bool addFieldsValueToTbl(const QString &strTbl, const QString &strFld, const T &t, const Args &... args)
    {
        QStringList lstFlds;
        QList<QVariant> lstVars;
        lstFlds << strFld;
        lstVars.push_back(t);//equals to .append(t)
        int arr[] = {(extractArg1(lstFlds, lstVars, args), 0)...};
        //--------------------------------------
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

    //update fields and values to a table ,dont create any new records.
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
    //=================================================


    QSqlDatabase *m_pDB;
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
};
#endif
