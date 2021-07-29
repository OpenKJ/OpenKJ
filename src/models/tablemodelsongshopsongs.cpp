#include "tablemodelsongshopsongs.h"
#include <QApplication>
#include <utility>
TableModelSongShopSongs::TableModelSongShopSongs(std::shared_ptr<SongShop> songShop, QObject *parent)
    : QAbstractTableModel(parent), shop(std::move(songShop))
{
    songs = shop->getSongs();
    connect(shop.get(), SIGNAL(songUpdateStarted()), this, SLOT(songShopUpdating()));
    connect(shop.get(), SIGNAL(songsUpdated()), this, SLOT(songShopUpdated()));
   // while (songs.isEmpty())
   //     QApplication::processEvents();
}

QVariant TableModelSongShopSongs::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
    QString txt;
        if (section == 0)
            txt = QString("Artist");
        if (section == 1)
            txt = QString("Title");
        if (section == 2)
            txt = QString("SongID");
        if (section == 3)
            txt = QString("Vendor");
        if (section == 4)
            txt = QString("Format");
        if (section == 5)
            txt = QString("Price");
        return QVariant(txt);
    }
    else
        return QVariant();
}

int TableModelSongShopSongs::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return songs.size();
}

int TableModelSongShopSongs::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 6;
}

QVariant TableModelSongShopSongs::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.column() == 5 && role == Qt::TextAlignmentRole)
        return Qt::AlignRight;
    if (index.column() == 4 && role == Qt::TextAlignmentRole)
        return Qt::AlignHCenter;
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
        {
            if (songs.at(index.row()).vendor == "Party Tyme Karaoke")
                return "mp3+g";
            else
                return "mp4";
        }
        if (index.column() == 5)
            return "$" + QString::number(songs.at(index.row()).price);
    }
    if (role == Qt::UserRole)
    {
        return songs.at(index.row()).songid;
    }
    return QVariant();
}

void TableModelSongShopSongs::songShopUpdating()
{
    emit layoutAboutToBeChanged();
}

void TableModelSongShopSongs::songShopUpdated()
{
    songs = shop->getSongs();
    emit layoutChanged();
}


SortFilterProxyModelSongShopSongs::SortFilterProxyModelSongShopSongs(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool SortFilterProxyModelSongShopSongs::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (searchTerms == "")
        return true;
    QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
    QModelIndex index1 = sourceModel()->index(source_row, 1, source_parent);
    QModelIndex index2 = sourceModel()->index(source_row, 2, source_parent);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList terms = searchTerms.split(" ", QString::SkipEmptyParts);
#else
    QStringList terms = searchTerms.split(' ', Qt::SplitBehavior(Qt::SkipEmptyParts));
#endif
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

void SortFilterProxyModelSongShopSongs::setSearchTerms(const QString &value)
{
    searchTerms = value;
    setFilterRegExp("");
}
