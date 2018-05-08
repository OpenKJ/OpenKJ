#ifndef SHOPSORTFILTERPROXYMODEL_H
#define SHOPSORTFILTERPROXYMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>

class ShopSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    ShopSortFilterProxyModel(QObject *parent = 0);

    // QSortFilterProxyModel interface
    void setSearchTerms(const QString &value);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
    QString searchTerms;
};

#endif // SHOPSORTFILTERPROXYMODEL_H
