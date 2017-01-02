/************************************************************************
* DBWrap
* 1、封装数据库的的打开关闭接口。
* 2、封装数据库的数据查询修改接口。（含读写互斥锁）
* 3、封装数据库拷贝接口。（运行时建立内存数据库）
* 4、含有一个qSqlDatabase指针
* Author:linbin
* Email:13918814219@163.com
************************************************************************/

#include "dbwrap.h"
#include "sqlite3.h"

DBWrap::DBWrap(const QString strType /* = "sqlite"*/)
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

DBWrap::~DBWrap()
{
    qDeleteAll(__lstQrys);

    m_pDB->close();
    delete m_pDB;
    QSqlDatabase::removeDatabase(__strConnName);
}

bool DBWrap::openDB(const QString strDBname)
{
    return openDB("", strDBname);
}

bool DBWrap::openDB(const QString strSvrName,
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

bool DBWrap::__openMDB(const QString strMDBname, QString strUserID, const QString strPassword)
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

bool DBWrap::__openMDBByUDL(const QString strUDL)
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

bool DBWrap::__connectSqlServer(const QString strSvrName, const QString strDBname, const QString strUserID, const QString strUserPwd)
{
    const QString dsn = QString("DRIVER={SQL Server};Server=%1;Database=%2;UID=%3;PWD=%4").arg(strSvrName).arg(strDBname).arg(strUserID).arg(strUserPwd);
    m_pDB->setDatabaseName(dsn);
    const bool b = m_pDB->open();
    //if(!b)  QMessageBox::critical(0, "error", m_dbConn.lastError().text());// m_dbConn.lastError().text();

    return b;
}

bool DBWrap::__openSqlite(const QString &strDbName)
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

bool DBWrap::__openMySql(QString strSvrName,
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

bool DBWrap::excuteSQL(const QString strExcutePara)
{
    QSqlQuery query(*m_pDB);
    query.clear();
    const bool b = query.exec(strExcutePara);
    return b;
}

bool DBWrap::dropTable(const QString strTbl)
{
    return excuteSQL(QString("DROP TABLE %1").arg(strTbl));
}

bool DBWrap::isOpen() const
{
    return m_pDB->isOpen();
}

bool DBWrap::isTableExist(const QString strTable)
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

void DBWrap::closeDB()
{
    m_pDB->close();
}

void *DBWrap::openRecordsetBySql(const QString strSql)
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

void DBWrap::closeRecordset(void *p)
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

bool DBWrap::recEOF(void *p) const
{
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return true;
    return (QSql::AfterLastRow == pQry->at());//Returns the current internal position of the query.
}

bool DBWrap::recBOF(void *p) const
{
    QSqlQuery *pQry = static_cast<QSqlQuery *>(p);
    if(nullptr == pQry)
        return true;
    return (QSql::BeforeFirstRow == pQry->at());
}

bool DBWrap::moveFirst(void *p) const
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

bool DBWrap::movePrevious(void *p) const
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

bool DBWrap::moveNext(void *p) const
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

bool DBWrap::moveLast(void *p) const
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

bool DBWrap::moveTo(int n, void *p) const
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

long DBWrap::getRecordCount(void *p) const
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

bool DBWrap::sqliteDBMemFile(QString filename, bool save ){

    bool state = false;


    if(filename.isNull() || filename.isEmpty() ) return state;
    if(__strDbType != "SQLITE" ) return state;


    QVariant v = m_pDB->driver()->handle();
    if (v.isValid() && qstrcmp(v.typeName(), "sqlite3*")==0) {
        // v.data() returns a pointer to the handle
        sqlite3 *handle = *static_cast<sqlite3 **>(v.data());
        if (handle != 0) { // check that it is not NULL
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

bool DBWrap::transaction()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->transaction();
    return true;
}

bool DBWrap::commit()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->commit();
    return true;
}

bool DBWrap::rollback()
{
    if (false == m_pDB->driver()->hasFeature(QSqlDriver::Transactions))
    {
        return false;
    }
    m_pDB->rollback();
    return true;
}
