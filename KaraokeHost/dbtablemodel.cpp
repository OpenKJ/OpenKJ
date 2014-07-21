#include "dbtablemodel.h"
#include <QMimeData>
#include <QByteArray>
#include <QStringList>

DbTableModel::DbTableModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    setTable("dbsongs");
    sortColumn = SORT_ARTIST;
    artistOrder = "ASC";
    titleOrder = "ASC";
    discIdOrder = "ASC";
    durationOrder = "ASC";
    select();
}


QMimeData *DbTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/songid", data(indexes.at(0).sibling(indexes.at(0).row(), 0)).toByteArray().data());
    return mimeData;
}

Qt::ItemFlags DbTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void DbTableModel::search(QString searchString)
{
    QStringList terms;
    terms = searchString.split(" ",QString::SkipEmptyParts);
    if (terms.size() < 1)
    {
        setFilter("");
        return;
    }
    QString whereClause = "filename LIKE \"%" + terms.at(0) + "%\"";
    for (int i=1; i < terms.size(); i++)
    {
        whereClause = whereClause + " AND filename LIKE \"%" + terms.at(i) + "%\"";
    }
    setFilter(whereClause);
}

QString DbTableModel::orderByClause() const
{
    QString sql = " ORDER BY ";
    switch (sortColumn) {
    case SORT_ARTIST:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", discid " + discIdOrder + ", duration " + durationOrder);
        break;
    case SORT_TITLE:
        sql.append("title " + titleOrder + ", artist " + artistOrder + ", discid " + discIdOrder + ", duration " + durationOrder);
        break;
    case SORT_DISCID:
        sql.append("discid " + discIdOrder + ", artist " + artistOrder + ", title " + titleOrder + ", duration " + durationOrder);
        break;
    case SORT_DURATION:
        sql.append("duration " + durationOrder + ", title " + titleOrder + ", artist " + artistOrder + ", discid " + discIdOrder);
        break;
    default:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", discid " + discIdOrder + ", duration " + durationOrder);
        break;
    }
    return sql;
}

void DbTableModel::sort(int column, Qt::SortOrder order)
{
    QString orderString = "ASC";
    if (order == Qt::DescendingOrder)
        orderString = "DESC";
    sortColumn = column;
    switch (sortColumn) {
    case SORT_ARTIST:
        artistOrder = orderString;
        break;
    case SORT_TITLE:
        titleOrder = orderString;
        break;
    case SORT_DISCID:
        discIdOrder = orderString;
        break;
    case SORT_DURATION:
        durationOrder = orderString;
        break;
    }
    select();
}
