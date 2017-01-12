#include <QCoreApplication>
#include "dbwrap.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DBWrap processor("sqlite");

    quint32 i = 100;
    quint32 j = 299;

    processor.openDB("test.db");
    processor.transaction();
    processor.excuteSQL("CREATE TABLE test(sss INTEGER,fff INTEGER, asd TEXT, PRIMARY KEY(sss));");
    processor.addFieldsValueToTbl("test","sss",i,"fff",j,"asd",QString("wewe"));

    processor.readFieldsValueFromRec()
    processor.commit();
    processor.closeDB();

//    return a.exec();
}
