#ifndef TABLEMODELROTATION_H
#define TABLEMODELROTATION_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QFont>
#include <QImage>
#include <QItemDelegate>
#include <QPainter>

struct RotationSinger {
    int id{0};
    QString name;
    int position{0};
    bool regular{false};
    QDateTime addTs;
};

class ItemDelegateRotation : public QItemDelegate
{
    Q_OBJECT
private:
    int m_currentSinger{-1};
    QImage m_iconDelete;
    QImage m_iconCurSinger;
    QImage m_iconRegularOn;
    QImage m_iconRegularOff;
    int m_curFontHeight;
    void resizeIconsForFont(const QFont &font);
public:
    explicit ItemDelegateRotation(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int currentSinger() const;
    void setCurrentSinger(const int singerId);
};

class TableModelRotation : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_NAME,COL_POSITION,COL_NEXT_SONG,COL_REGULAR,COL_ADDTS,COL_DELETE};
    enum {ADD_FAIR=0,ADD_BOTTOM,ADD_NEXT};
    explicit TableModelRotation(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void loadData();

    void commitChanges();
    int singerAdd(const QString& name, const int positionHint = ADD_BOTTOM);
    void singerMove(const int oldPosition, const int newPosition, const bool skipCommit = false);
    void singerSetName(const int singerId, const QString &newName);
    void singerDelete(const int singerId);
    bool singerExists(const QString &name);
    bool singerIsRegular(const int singerId);
    void singerSetRegular(const int singerId, const bool isRegular);
    void singerMakeRegular(const int singerId);
    uint singerTurnDistance(const int singerId);
    void singerDisableRegularTracking(const int singerId);
    bool historySingerExists(const QString &name);
    QString getSingerName(const int singerId);
    int getSingerId(const QString &name);
    int getSingerPosition(const int singerId) const;
    int singerIdAtPosition(int position) const;
    QStringList singers();
    QStringList historySingers() const;
    QString nextSongPath(const int singerId) const;
    QString nextSongArtist(const int singerId) const;
    QString nextSongTitle(const int singerId) const;
    QString nextSongArtistTitle(const int singerId) const;
    QString nextSongSongId(const int singerId) const;
    int nextSongDurationSecs(const int singerId) const;
    int rotationDuration();
    int nextSongKeyChg(const int singerId) const;
    int nextSongQueueId(const int singerId) const;
    void clearRotation();
    int currentSinger() const;
    void setCurrentSinger(const int currentSingerId);
    void setRotationTopSingerId(const int id);
    bool rotationIsValid();
    int numSongs(const int singerId) const;
    int numSongsSung(const int singerId) const;
    int numSongsUnsung(const int singerId) const;
    QDateTime timeAdded(const int singerId);
    void outputRotationDebug();
    void fixSingerPositions();
    void resizeIconsForFont(const QFont &font);
    void setCurRemainSecs(const int secs) { m_remainSecs = secs; }

private:
    std::vector<RotationSinger> m_singers;
    int m_currentSingerId{-1};
    int m_rotationTopSingerId{-1};
    QImage m_iconGreenCircle;
    QImage m_iconYellowCircle;
    int m_curFontHeight;
    int m_remainSecs{0};

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void regularAddNameConflict(QString name);
    void regularLoadNameConflict(QString name);
    void rotationModified();
    void regularsModified();
    void singersMoved(const int startRow, const int startCol, const int endRow, const int endCol);

    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // TABLEMODELROTATION_H
