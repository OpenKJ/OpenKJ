#ifndef SONGSHOPMODEL_H
#define SONGSHOPMODEL_H

#include <QAbstractTableModel>
#include "songshop.h"

class SongShopModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SongShopModel(SongShop *songShop, QObject *parent = 0);

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
