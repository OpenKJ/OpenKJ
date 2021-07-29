#ifndef SONGSHOPMODEL_H
#define SONGSHOPMODEL_H

#include <QAbstractTableModel>
#include "songshop.h"
#include <QSortFilterProxyModel>
#include <memory>

class SortFilterProxyModelSongShopSongs : public QSortFilterProxyModel
{
public:
    explicit SortFilterProxyModelSongShopSongs(QObject *parent = nullptr);
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
    explicit TableModelSongShopSongs(std::shared_ptr<SongShop> songShop, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    std::shared_ptr<SongShop> shop;
    ShopSongs songs;

private slots:
    void songShopUpdating();
    void songShopUpdated();

};

#endif // SONGSHOPMODEL_H
