#ifndef TABLEMODELQUEUESONGSNEW_H
#define TABLEMODELQUEUESONGSNEW_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QModelIndex>
#include <QPainter>
#include <QUrl>
#include "tablemodelkaraokesongs.h"

struct QueueSong {
    int id{0};
    int singerId{0};
    int dbSongId{0};
    bool played{false};
    int keyChange{0};
    int position{0};
    QString artist;
    QString title;
    QString songId;
    int duration{0};
    QString path;
};

class ItemDelegateQueueSongs : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSong;
    QImage m_iconDelete;
    QImage m_iconPlaying;
    int m_curFontHeight;
    void resizeIconsForFont(const QFont &font);
public:
    explicit ItemDelegateQueueSongs(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class TableModelQueueSongs : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_DBSONGID,COL_ARTIST,COL_TITLE,COL_SONGID,COL_KEY,COL_DURATION,COL_PATH};
    explicit TableModelQueueSongs(TableModelKaraokeSongs &karaokeSongsModel, QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void loadSinger(const int singerId);
    int getSingerId() const { return m_curSingerId; }
    int getPosition(const int songId);
    bool getPlayed(const int songId);
    int getKey(const int songId);
    void move(const int oldPosition, const int newPosition);
    void moveSongId(const int songId, const int newPosition);
    int add(const int songId);
    void insert(const int songId, const int position);
    void remove(const int songId);
    void setKey(const int songId, const int semitones);
    void setPlayed(const int qSongId, const bool played = true);
    void removeAll();
    void commitChanges();

private:
    int m_curSingerId{0};
    TableModelKaraokeSongs &m_karaokeSongsModel;
    std::vector<QueueSong> m_songs;

signals:
    void queueModified(int singerId);
    void songDroppedWithoutSinger();
    void filesDroppedOnSinger(QList<QUrl> urls, int singerId, int position);
    void qSongsMoved(const int startRow, const int startCol, const int endRow, const int endCol);

public slots:
    void songAddSlot(int songId, int singerId, int keyChg = 0);

};

#endif // TABLEMODELQUEUESONGSNEW_H
