#include "songshopmodel.h"
#include <QApplication>
SongShopModel::SongShopModel(SongShop *songShop, QObject *parent)
    : QAbstractTableModel(parent)
{
    shop = songShop;
    songs = shop->getSongs();
    connect(shop, SIGNAL(songUpdateStarted()), this, SLOT(songShopUpdating()));
    connect(shop, SIGNAL(songsUpdated()), this, SLOT(songShopUpdated()));
   // while (songs.isEmpty())
   //     QApplication::processEvents();
}

QVariant SongShopModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
    QString txt;
        if (section == 0)
            txt = QString("Artist");
        if (section == 1)
            txt = QString("Title");
        if (section == 2)
            txt = QString("SongId");
        if (section == 3)
            txt = QString("Vendor");
        if (section == 4)
            txt = QString("Price");
        return QVariant(txt);
    }
    else
        return QVariant();
}

int SongShopModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return songs.size();
}

int SongShopModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 5;
}

QVariant SongShopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.column() == 4 && role == Qt::TextAlignmentRole)
        return Qt::AlignRight;
    // FIXME: Implement me!
    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
            return songs.at(index.row()).artist;
        if (index.column() == 1)
            return songs.at(index.row()).title;
        if (index.column() == 2)
            return songs.at(index.row()).songid;
        if (index.column() == 3)
            return songs.at(index.row()).vendor;
        if (index.column() == 4)
            return "$" + QString::number(songs.at(index.row()).price);
    }
    if (role == Qt::UserRole)
    {
        return songs.at(index.row()).songid;
    }
    return QVariant();
}

void SongShopModel::songShopUpdating()
{
    emit layoutAboutToBeChanged();
}

void SongShopModel::songShopUpdated()
{
    songs = shop->getSongs();
    emit layoutChanged();
}
