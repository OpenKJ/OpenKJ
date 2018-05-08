#include "shopsortfilterproxymodel.h"

ShopSortFilterProxyModel::ShopSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool ShopSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (searchTerms == "")
        return true;
    QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
    QModelIndex index1 = sourceModel()->index(source_row, 1, source_parent);
    QModelIndex index2 = sourceModel()->index(source_row, 2, source_parent);
    QStringList terms = searchTerms.split(" ", QString::SkipEmptyParts);
    QString artist = sourceModel()->data(index0).toString();
    QString title = sourceModel()->data(index1).toString();
    QString songId = sourceModel()->data(index2).toString();
    foreach (QString term, terms)
    {
        if (!artist.contains(term, Qt::CaseInsensitive) && !title.contains(term, Qt::CaseInsensitive) && !songId.contains(term, Qt::CaseInsensitive))
            return false;
    }
    return true;
}

void ShopSortFilterProxyModel::setSearchTerms(const QString &value)
{
    searchTerms = value;
    setFilterRegExp("");
}
