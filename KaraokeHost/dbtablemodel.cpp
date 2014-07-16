#include "dbtablemodel.h"
#include <QMimeData>
#include <QByteArray>

DbTableModel::DbTableModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
}


QMimeData *DbTableModel::mimeData(const QModelIndexList &indexes) const
{
//    Q_UNUSED(indexes);
//    QMimeData *mimeData = new QMimeData();
//    QByteArray output;
//    QBuffer outputBuffer(&output);
//    int songid = filteredData->at(indexes.at(0).row())->ID;
//    outputBuffer.open(QIODevice::WriteOnly);
//    outputBuffer.write(QString::number(songid).toLocal8Bit());
////    outputBuffer.write("1");
//    mimeData->setData("integer/songid", output);
//    return mimeData;
    return new QMimeData();

}

Qt::ItemFlags DbTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}
