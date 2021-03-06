﻿// DBProcess.cpp :
/************************************************************************
*Description:                                                           *
*Author: realfan                                                        *
*Date of Created:                                                       *
*Date of Modified 2016.12.07 Purpose of Modify Person Name of Modify    *
*------------------  ----------------------  -------------------------  *
*                                                                       *
************************************************************************/
#include "DBProcess.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
//#include <QMessageBox>
#include <QSqlError>
#include <QFileInfo>

#include "sqlite3.h"
//=================================
CDBProcess::CDBProcess(const QString strType /* = "sqlite"*/)
{
    __strDbType = strType.toUpper();
    QString dbType("SQLITE");
    if("SQLITE" == __strDbType)
        dbType = "QSQLITE"; //QTPLUGIN += qsqlite Q_IMPORT_PLUGIN(QSQLiteDriverPlugin)
    else if ("MYSQL" == __strDbType)
        dbType = "QMYSQL"; //QTPLUGIN += qsqlmysql Q_IMPORT_PLUGIN(QMYSQLDriverPlugin)
    else if ("SQLSERVER" == __strDbType)
        dbType = "QODBC"; //QTPLUGIN += qsqlodbc  Q_IMPORT_PLUGIN(QODBCDriverPlugin)
    else if("ACCESS" == __strDbType)
        dbType = "QODBC"; //QTPLUGIN += qsqlodbc  Q_IMPORT_PLUGIN(QODBCDriverPlugin)
    else
    {
        dbType = "";
//        QMessageBox::critical(0, "ERROR", "DB type name invalid!");
        return;
    }
    int iConnIdx = 0;
    while(1)
    {
        __strConnName = QString("MyDBProcessConn%1").arg(++iConnIdx);
        QSqlDatabase dbConn = QSqlDatabase::database(__strConnName, false);
        //to avoid connName confliction
        if(dbConn.isValid())//存在连接
        {
            continue;
        }
        m_pDB = new QSqlDatabase(QSqlDatabase::addDatabase(dbType, __strConnName));
        break;
    }
}
CDBProcess::~CDBProcess()
{
    qDeleteAll(__lstQrys);

    m_pDB->close();
    delete m_pDB;
    QSqlDatabase::removeDatabase(__strConnName);
}
bool CDBProcess::openDB(const QString strDBname)
{
    return openDB("", strDBname);
}

bool CDBProcess::openDB(const QString strSvrName,
                        const QString strDBname,
                        const QString strUserID,
                        const QString strUserPwd)
{
    if(isOpen())
        return false;
    __strDbName = strDBname;
    bool bRet = false;
    if("SQLITE" == __strDbType)
    {
        bRet = __openSqlite(strDBname);
    }
    else if("MYSQL" == __strDbType)
    {
        bRet = __openMySql(strSvrName, strDBname, strUserID, strUserPwd);
    }
    else if("SQLSERVER" == __strDbType)
    {
        bRet = __connectSqlServer(strSvrName, strDBname, strUserID, strUserPwd);
    }
    else if("ACCESS" == __strDbType)
    {
        if(strSvrName.length() < 1)
        {
            bRet = __openMDB(strDBname, strUserID, strUserPwd);
        }
        else
        {
            bRet = __openMDBByUDL(strSvrName); //here strSvrName is UDL file name
        }
    }

    return bRet;
}
bool CDBProcess::__openMDB(const QString strMDBname, QString strUserID, const QString strPassword)
{
    QString strMDB = strMDBname.trimmed();
    if(strUserID.isEmpty())
    {
        strUserID = "sa";
    }
    if(QFileInfo(strMDB).fileName() == strMDB)
    {
        //no path, only mdb file name
        strMDB = QString("%1/%2").arg( QCoreApplication::applicationDirPath() ).arg( strMDB );
    }
    //access  "Driver={microsoft access driver(*.mdb)};dbq=*.mdb;uid=admin;pwd=pass;"
    //"DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=c:/tt/b.accdb;UID=admin;PWD=a"
    //for win7
    //QString dsn = QString("DRIVER={Microsoft Access Driver (*.mdb, *.accdb)};DBQ=%1;UID=%2;PWD=%3").arg(strMDBname).arg(strUserID).arg(strPassword);
    //for winxp
    const QString dsn = QString("DRIVER={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=%1;UID=%2;PWD=%3").arg(strMDB).arg(strUserID).arg(strPassword);
    m_pDB->setDatabaseName(dsn);
    const bool b = m_pDB->open();
    if(false == b)
    {
//        QMessageBox::critical(0, "ERROR", m_pDB->lastError().text());
    }
    return b;
}
bool CDBProcess::__openMDBByUDL(const QString strUDL)
{
    QFile file(strUDL);
    if(!file.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString strAll = in.readAll();
    if(strAll.length() > 3 && QChar(0) == strAll[1])
    {
        in.setCodec("UTF-16");
        in.seek(0);
        strAll = in.readAll();
    }
    file.close();
    int idx1 = strAll.indexOf("=");
    idx1 = strAll.indexOf("=", idx1 + 1);
    int idx2 = strAll.indexOf(";", idx1 + 1);
    QString strMDB = strAll.mid(idx1 + 1, idx2 - idx1 - 1);
    strMDB = strMDB.trimmed();

    if(QFileInfo(strMDB).fileName() == strMDB)
    {
        strMDB = QString("%1/%2").arg( QCoreApplication::applicationDirPath() ).arg( strMDB );
    }
    return __openMDB(strMDB);
}
bool CDBProcess::__connectSqlServer(const QString strSvrName, const QString strDBname, const QString strUserID, const QString strUserPwd)
{
    const QString dsn = QString("DRIVER={SQL Server};Server=%1;Database=%2;UID=%3;PWD=%4").arg(strSvrName).arg(strDBname).arg(strUserID).arg(strUserPwd);
    m_pDB->setDatabaseName(dsn);
    const bool b = m_pDB->open();
    //if(!b)  QMessageBox::critical(0, "error", m_dbConn.lastError().text());// m_dbConn.lastError().text();

    return b;
}
bool CDBProcess::__openSqlite(const QString &strDbName)
{
    QString strDB = strDbName.trimmed();
    if(QFileInfo(strDB).fileName() == strDB)
    {
        strDB = QString("%1/%2").arg( QCoreApplication::applicationDirPath() ).arg( strDB );
    }
    m_pDB->setDatabaseName(strDB);
    if (!m_pDB->open())
    {
        //qDebug() << " can’t open database >>>>>> mydb.db";
        return false;
    }
    return true;
}
bool CDBProcess::__openMySql(QString strSvrName,
                             QString strDBname,
                             QString strUserID,
                             const QString strUserPwd)
{
    if(strSvrName.length() < 1) strSvrName = "localhost";
    if(strUserID.length() < 1) strUserID = "root";
    if(strDBname.length() < 1) return false;

    m_pDB->setHostName(strSvrName);
    m_pDB->setDatabaseName(strDBname);
    m_pDB->setUserName(strUserID);//"root");
    m_pDB->setPassword(strUserPwd);
    m_pDB->exec("SET NAMES 'utf8'");
    if(false == m_pDB->open())
    {
        return false;
    }
    return true;
}

/********************************************************************/
bool CDBProcess::excuteSQL(const QString strExcutePara)
{
    QSqlQuery query(*m_pDB);
    query.clear();
    const bool b = query.exec(strExcutePara);
    return b;
}
bool CDBProcess::dropTable(const QString strTbl)
{
    return excuteSQL(QString("DROP TABLE %1").arg(strTbl));
}
/************************************************************************
*Function Name: isOpen                                                  *
*Description:   判断数据库是否关闭                                         *
*Param of Input:                                                        *
*               No                                                      *
*Param of Output:                                                       *
*               NO                                                      *
*Output:                                                                *
*               bool                                                    *
*Author: Donny Wang                                                     *
*Date of Created:                                                       *
*Date of Modified      Purpose of Modify       Person Name of Modify    *
*------------------  ----------------------  -------------------------  *
*                                                                       *
************************************************************************/
bool CDBProcess::isOpen() const
{
    return m_pDB->isOpen();
}

bool CDBProcess::isTableExist(const QString strTable)
{
    bool bRet = false;
    if("SQLITE" == __strDbType)
    {
        const QString str = QString("select * from sqlite_master where type='table' and name='%1'").arg(strTable);
        void *pRet = openRecordsetBySql(str);
        if(pRet && moveNext(pRet))
        {
            closeRecordset(pRet);
            return true;
        }
        else
        {
            return false;
        }
    }
    else if("MYSQL" == __strDbType)
    {
        //selct table_name from INFORMATION_SCHEMA.TABLES where table_name 't5' and TABLE-SCHEMA='test';
        /*SELECT count( * )
        FROM information_schema.TABLES
        WHERE table_name = 'table name'
        AND TABLE_SCHEMA = 'database name'*/
    }

    return bRet;
}

/********************************************************************
*    Function Name: CloseDatabase                                   *
*    Function Description: 获取记录　　　　　　　                          *
*    Param In:      Void                                            *
*    Param Out:     Void                                            *
*    Return Value:  Void                                            *
*    Created:        10:15:2006   19:16                             *
*    Author:                                                        *
*    E-Mail:                                                        *
*    -----------------------------------------------------------    *
*    Purpose:                                                       *
********************************************************************/
void CDBProcess::closeDB()
{
    m_pDB->close();
}

/********************************************************************
*    Function Name: OpenRecordsetBySql                              *
*    Function Description: 获取记录　　　　　　　                          *
*    Param In:      Void                                            *
*    Param Out:     Void                                            *
*    Return Value:  Void                                            *
*    Created:        12:15:2006                                     *
*    Author:                                                        *
*    E-Mail:        wdg@163.com                                     *
*    -----------------------------------------------------------    *
*    Purpose:                                                       *
********************************************************************/
void *CDBProcess::openRecordsetBySql(const QString strSql)
{
    if (false == isOpen())
    {
        return nullptr;
    }
    QSqlQuery *pQry = new QSqlQuery(*m_pDB);
    pQry->clear();
    const bool b = pQry->exec(strSql);
    if(!b)
    {
        delete pQry;
        pQry = nullptr;
    }
    __lstQrys << pQry;
    return pQry;
}
void CDBProcess::closeRecordset(void *p)
{
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(pQry)
    {
        __lstQrys.removeOne(pQry);
        pQry->clear();
        delete pQry;
        pQry = nullptr;
    }
}
bool CDBProcess::recEOF(void *p) const
{
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return true;
    return (QSql::AfterLastRow == pQry->at());//Returns the current internal position of the query.
}
bool CDBProcess::recBOF(void *p) const
{
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return true;
    return (QSql::BeforeFirstRow == pQry->at());
}
bool CDBProcess::moveFirst(void *p) const
{
    if (false == isOpen())
    {
        return false;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return false;
    if(pQry->isActive())
    {
        return pQry->first();
    }
    else
    {
        return false;
    }
}

bool CDBProcess::movePrevious(void *p) const
{
    if (false == isOpen())
    {
        return false;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return false;
    if(pQry->isActive())
    {
        return pQry->previous();
    }
    else
    {
        return false;
    }
}
bool CDBProcess::moveNext(void *p) const
{
    if (false == isOpen())
    {
        return false;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return false;
    if(pQry->isActive())
    {
        return pQry->next();
    }
    else
    {
        return false;
    }
}
bool CDBProcess::moveLast(void *p) const
{
    if (false == isOpen())
    {
        return false;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return false;
    if(pQry->isActive())
    {
        return pQry->last();
    }
    else
    {
        return false;
    }
}
bool CDBProcess::moveTo(int n, void *p) const
{
    if (false == isOpen())
    {
        return false;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return false;
    if(pQry->isActive())
    {
        return pQry->seek(n);
    }
    else
    {
        return false;
    }
}

long CDBProcess::getRecordCount(void *p) const
{
    if (false == isOpen())
    {
        return -1;
    }
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return -1;
    if(m_pDB->driver()->hasFeature(QSqlDriver::QuerySize))
    {
        return pQry->size();
    }
    else
    {
        int i = pQry->at();
        pQry->last();
        int iRows = pQry->at() + 1;
        pQry->seek(i);
        return iRows;
    }
}

/*
 * sqliteDBMemFile 针对sqlite3内存型数据库的 save和load封装
 * 输入参数：
 * 参数1：QString filename 文件名称
 * 参数2：bool save true保存到文件，false从文件读取
 * 返回数值：
 * 1、成功true
 * 2、失败false （不是sqlite数据库，filename不成立）
 * 功能描述：
 * 1、数据库保存和读取
 */
bool CDBProcess::sqliteDBMemFile(QString filename, bool save ){

    bool state = false;


    if(filename.isNull() || filename.isEmpty() ) return state;
    if(__strDbType != "SQLITE" ) return state;


    QVariant v = m_pDB->driver()->handle();
    if( v.isValid() && qstrcmp(v.typeName(),"sqlite3*") == 0 )
    {
        // v.data() returns a pointer to the handle
        sqlite3 * handle = *static_cast(v.data());
        if( handle != 0 ) // check that it is not NULL
        {
            sqlite3 * pInMemory = handle;
            const char * zFilename = filename.toLocal8Bit().data();
            int rc; /* Function return code */
            sqlite3 *pFile; /* Database connection opened on zFilename */
            sqlite3_backup *pBackup; /* Backup object used to copy data */
            sqlite3 *pTo; /* Database to copy to (pFile or pInMemory) */
            sqlite3 *pFrom; /* Database to copy from (pFile or pInMemory) */

            rc = sqlite3_open( zFilename, &pFile );
            if( rc==SQLITE_OK ){
                pFrom = ( save ? pInMemory : pFile);
                pTo = ( save ? pFile : pInMemory);

                pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
                if( pBackup ){
                        (void)sqlite3_backup_step(pBackup, -1);
                        (void)sqlite3_backup_finish(pBackup);
                }
                rc = sqlite3_errcode(pTo);
            }

            (void)sqlite3_close(pFile);

            if( rc == SQLITE_OK ) state = true;

        }

    }
    return state;
}

#if 0
//"id%+s|name%-s|age%-d|pret%*d", 15, pch, &i, &iRet
bool CDBProcess::exexProc(const QString strStoreProc, QString str1, QString &str2)
{
    QSqlQuery *pQry = (QSqlQuery *)m_mapQry[-1];
    if(nullptr == pQry)
    {
        m_mapQry.remove(-1);

        pQry = new QSqlQuery(*m_pDB);
        m_mapQry.insert(-1, pQry);
    }
    pQry->clear();
    QString strSql = "exec " + strStoreProc + " ?, ? output";
    pQry->prepare(strSql);
    pQry->bindValue(0, str1);
    pQry->bindValue(1, 0, QSql::Out);
    bool b = pQry->exec();
    //    if(!b)
    //    {
    //        QString str = pQry->lastError().text();
    //    }

    str2 = b ? pQry->boundValue(1).toString() : "";

    return b;
}
bool CDBProcess::execStoreProcOfArgList(int idx, const QString strStoreProc, const char *szFldsInfo, ...)
{
    if (nullptr == szFldsInfo)
    {
        return false;
    }
    QSqlQuery *pQry = (QSqlQuery *)m_mapQry[idx];
    if(nullptr == pQry)
    {
        m_mapQry.remove(idx);

        pQry = new QSqlQuery(*m_pDB);
        m_mapQry.insert(idx, pQry);
    }
    pQry->clear();

    QString strFlds(szFldsInfo);
    QStringList strlst = strFlds.split("|");
    //QString strSql = QString("CALL %1").arg(strStoreProc) ;
    QString strSql = QString("exec %1").arg(strStoreProc) ;
    for(int i = 0; i < strlst.size(); ++i)
    {
        if(0 == i)
            strSql += " ?";
        else if(strlst.size() - 1 == i)
            strSql += ",?";
        else
            strSql += ",?";
    }
    pQry->prepare(strSql);


    va_list arg_ptr;
    va_start(arg_ptr, szFldsInfo);
    //pQry->prepare(strSql);//("INSERT INTO person (id, forename, surname) VALUES (:id, :forename, :surname)");
    //bool bExed = false;
    for (int i = 0; i < strlst.size(); ++i)
    {
        //-表示输出 +或无表示输入 *表示return  //"id%+s|name%-s|age%-d|pret%*d", 15, pch, &i, &iRet
        QString strFldInfo = strlst.at(i);
        QString strFldName = strFldInfo.mid(0, strFldInfo.indexOf('%'));//get field name
        QChar ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 1, 1)).at(0);
        QSql::ParamTypeFlag eParamTypeFlag = QSql::In;
        switch (ch.toLatin1())
        {
        case L'-':
            eParamTypeFlag = QSql::Out;
            ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 2, 1)).at(0);
            break;
        case '+':
            eParamTypeFlag = QSql::In;
            ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 2, 1)).at(0);
            break;
        default:
            eParamTypeFlag = QSql::In;
        }
        if(QSql::In == eParamTypeFlag)
        {
            switch (ch.toLatin1())
            {
            case 'i':
            {
                //64bit整数
                qint64 k = va_arg(arg_ptr, qint64);
                pQry->bindValue(i, k);
                break;
            }
            case 'd':
            case 'u':
            {
                //整数
                int k = va_arg(arg_ptr, int);
                pQry->bindValue(i, k);
                break;
            }
            case 'f':
            case 'g':
            {
                //小数double类型,无float类型
                double d = va_arg(arg_ptr, double);
                pQry->bindValue(i, d);
                break;
            }
            case 's':
            {
                //文本
                //char *pstr = va_arg(arg_ptr, char*);
                QString *pch = va_arg(arg_ptr, QString *);
                QString strVal = *pch;
                pQry->bindValue(i, strVal);
                break;
            }
            case 'b':
            {
                //二进制
                QByteArray *pData = va_arg( arg_ptr, QByteArray * );
                pQry->bindValue(i, *pData);
                break;
            }
            case 't':
            {
                //时间日期
                QDateTime *pdt = va_arg( arg_ptr, QDateTime * );
                pQry->bindValue(i, *pdt);
                break;
            }
            } //end of switch
        }
        else if (QSql::Out == eParamTypeFlag)
        {
            va_arg(arg_ptr, void *); //shift arg
            pQry->bindValue(i, 0, QSql::Out);
        }
    } //end of for
    va_end(arg_ptr);
    if(false == pQry->exec())
    {
        QSqlError err = pQry->lastError();
        QString str = err.text();
        return false;
    }
    va_start(arg_ptr, szFldsInfo);
    for (int i = 0; i < strlst.size(); ++i)
    {
        //-表示输出 +或无表示输入 *表示return  //"id%+s|name%-s|age%-d|pret%*d", 15, pch, &i, &iRet
        QString strFldInfo = strlst.at(i);
        QString strFldName = strFldInfo.mid(0, strFldInfo.indexOf('%'));//get field name
        QChar ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 1, 1)).at(0);
        QSql::ParamTypeFlag eParamTypeFlag = QSql::In;
        switch (ch.toLatin1())
        {
        case L'-':
            eParamTypeFlag = QSql::Out;
            ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 2, 1)).at(0);
            break;
        case '+':
            eParamTypeFlag = QSql::In;
            ch = (strFldInfo.mid(strFldInfo.indexOf('%') + 2, 1)).at(0);
            break;
        default:
            eParamTypeFlag = QSql::In;
        }
        if(QSql::In == eParamTypeFlag)
        {
            switch (ch.toLatin1())
            {
            case 'i':
            {
                //整数
                va_arg(arg_ptr, qint64);
                //pQry->bindValue(i, k);
                break;
            }
            case 'd':
            case 'u':
            {
                //整数
                va_arg(arg_ptr, int);
                //pQry->bindValue(i, k);
                break;
            }
            case 'f':
            case 'g':
            {
                //小数double类型,无float类型
                va_arg(arg_ptr, double);
                //pQry->bindValue(i, d);
                break;
            }
            case 's'://文本
            case 'b'://二进制
            case 't'://时间日期
            {
                va_arg( arg_ptr, void * );
                break;
            }
            } //end of switch
        }
        else if (QSql::Out == eParamTypeFlag)
        {
            //int i = query.boundValue(1).toInt(); // i is 65
            void *pRet = va_arg(arg_ptr, void *); //shift arg
            QVariant var = pQry->boundValue(i);
            switch (ch.toLatin1())
            {
            case 'i':
            {
                //整数
                qint64 *pInt = (qint64 *)pRet;
                *pInt = var.toLongLong();
                break;
            }
            case 'd':
            case 'u':
            {
                //整数
                int *pInt = (int *)pRet;
                *pInt = var.toInt();
                break;
            }
            case 'B':
            {
                unsigned char *puc = (unsigned char *)pRet;
                *puc = (unsigned char)var.toInt();
                break;
            }
            case 'f':
            case 'g':
            {
                //小数double类型,无float类型
                double *pD = (double *)pRet;
                *pD = var.toDouble();
                break;
            }
            case 's':
            {
                //文本
                QString *pstr = (QString *)pRet;
                *pstr = var.toString();
                break;
            }
            case 'b':
            {
                //二进制
                QByteArray *pData = va_arg( arg_ptr, QByteArray * );
                *pData = var.toByteArray();
                break;
            }
            case 't':
            {
                //时间日期
                QDateTime *dt = va_arg( arg_ptr, QDateTime * );
                *dt = var.toDateTime();
                break;
            }
            } //end of switch
        }
    } //end of for

    return true;
}
#endif
//==============================================================

bool CDBProcess::transaction()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->transaction();
    return true;
}
bool CDBProcess::commit()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->commit();
    return true;
}
bool CDBProcess::rollback()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->rollback();
    return true;
}
