#include "songstablemodel.h"
#include <QMimeData>
#include <QStringList>

SongsTableModel::SongsTableModel(QObject *parent, QSqlDatabase db) :
    QSqlTableModel(parent, db)
{
    sortColumn = SORT_ARTIST;
    artistOrder = "ASC";
    titleOrder = "ASC";
    filenameOrder = "ASC";
}

Qt::ItemFlags SongsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void SongsTableModel::search(QString searchString)
{
    QStringList terms;
    terms = searchString.split(" ",QString::SkipEmptyParts);
    if (terms.size() < 1)
    {
        setFilter("4");
        return;
    }
    QString whereClause = "searchstring LIKE \"%" + terms.at(0) + "%\"";
    for (int i=1; i < terms.size(); i++)
    {
        whereClause = whereClause + " AND searchstring LIKE \"%" + terms.at(i) + "%\"";
    }
    setFilter(whereClause);
}


QMimeData *SongsTableModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("integer/songid", data(indexes.at(0).sibling(indexes.at(0).row(),0)).toByteArray().data());
    return mimeData;
}


QString SongsTableModel::orderByClause() const
{
    QString sql = " ORDER BY ";
    switch (sortColumn) {
    case SORT_ARTIST:
        sql.append("artist " + artistOrder + ", title " + titleOrder + ", filename " + filenameOrder);
        break;
    case SORT_TITLE:
        sql.append("title " + titleOrder + ", artist " + artistOrder + ", filename " + filenameOrder);
        break;
    case SORT_FILENAME:
        sql.append("filename " + filenameOrder + ", artist " + artistOrder + ", title " + titleOrder);
        break;
    }
    return sql;
}


void SongsTableModel::sort(int column, Qt::SortOrder order)
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
    case SORT_FILENAME:
        filenameOrder = orderString;
        break;
    }
    select();
}
