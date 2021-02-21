#ifndef SONGSHOPMODEL_H
#define SONGSHOPMODEL_H

#include <QAbstractTableModel>
#include "songshop.h"
#include <QSortFilterProxyModel>

class SortFilterProxyModelSongShopSongs : public QSortFilterProxyModel
{
public:
    SortFilterProxyModelSongShopSongs(QObject *parent = 0);
    void setSearchTerms(const QString &value);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QString searchTerms;
};

class TableModelSongShopSongs : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TableModelSongShopSongs(SongShop *songShop, QObject *parent = 0);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    SongShop *getShop() { return shop; }

private:
    SongShop *shop;
    ShopSongs songs;

private slots:
    void songShopUpdating();
    void songShopUpdated();

};

#endif // SONGSHOPMODEL_H
