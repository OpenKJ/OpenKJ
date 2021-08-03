#ifndef TABLEMODELQUEUESONGSNEW_H
#define TABLEMODELQUEUESONGSNEW_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QModelIndex>
#include <QPainter>
#include <QUrl>
#include "tablemodelkaraokesongs.h"
#include "settings.h"

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
    QImage m_iconDelete;
    int m_curFontHeight{0};
    Settings m_settings;

public:
    explicit ItemDelegateQueueSongs(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

public slots:
    void resizeIconsForFont(const QFont &font);

};

class TableModelQueueSongs : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum {COL_ID=0,COL_DBSONGID,COL_ARTIST,COL_TITLE,COL_SONGID,COL_KEY,COL_DURATION,COL_PATH};
    explicit TableModelQueueSongs(TableModelKaraokeSongs &karaokeSongsModel, QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;

    void loadSinger(int singerId);
    [[nodiscard]] int getSingerId() const { return m_curSingerId; }
    int getPosition(int songId);
    bool getPlayed(int songId);
    int getKey(int songId);
    void move(int oldPosition, int newPosition);
    void moveSongId(int songId, int newPosition);
    int add(int songId);
    void insert(int songId, int position);
    void remove(int songId);
    void setKey(int songId, int semitones);
    void setPlayed(int qSongId, bool played = true);
    void removeAll();
    void commitChanges();

private:
    int m_curSingerId{0};
    TableModelKaraokeSongs &m_karaokeSongsModel;
    std::vector<QueueSong> m_songs;
    Settings m_settings;

signals:
    void queueModified(int singerId);
    void songDroppedWithoutSinger();
    void filesDroppedOnSinger(QList<QUrl> urls, int singerId, int position);
    void qSongsMoved(int startRow, int startCol, int endRow, int endCol);

public slots:
    void songAddSlot(int songId, int singerId, int keyChg = 0);

};

#endif // TABLEMODELQUEUESONGSNEW_H
