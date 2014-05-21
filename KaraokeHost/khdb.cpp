#include "khdb.h"
#include <QSqlQuery>

KhDb::KhDb(QObject *parent) :
    QObject(parent)
{
}

void KhDb::beginTransaction()
{
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
}

void KhDb::endTransaction()
{
    QSqlQuery query;
    query.exec("COMMIT TRANSACTION");
}

bool KhDb::singerSetRegular(int singerId, bool value)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'regular'=" + QString::number(value) + " WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetPosition(int singerId, int position)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'position'=" + QString::number(position) + " WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetName(int singerId, QString name)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'name'=\"" + name + "\" WHERE ROWID == " + QString::number(singerId);
    return query.exec(sql);
}

bool KhDb::singerSetRegIndex(int singerId, int regId)
{
    QSqlQuery query;
    QString sql = "UPDATE rotationsingers SET 'regularid'=" + QString::number(regId) + " WHERE ROWID == " + QString::number(singerId);
    query.exec(sql);
}
