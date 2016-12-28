#include <QCoreApplication>
#include "DBProcess.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CDBProcess processor("sqlite");

    quint32 i = 100;
    quint32 j = 299;

    processor.openDB("test.db");
    processor.transaction();
    processor.excuteSQL("CREATE TABLE test(sss int,fff int,PRIMARY KEY(sss));");
    processor.addFieldsValueToTbl("test","sss",i,"fff",j);
    processor.commit();
    processor.closeDB();

//    return a.exec();
}
