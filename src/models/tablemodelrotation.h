#ifndef TABLEMODELROTATION_H
#define TABLEMODELROTATION_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QFont>
#include <QImage>
#include <QItemDelegate>
#include <QPainter>
#include <spdlog/async_logger.h>
#include <optional>
#include "settings.h"

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
    int m_curFontHeight{0};
    Settings m_settings;

public:
    explicit ItemDelegateRotation(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    [[nodiscard]] int currentSinger() const;
    void setCurrentSinger(int singerId);

public slots:
    void resizeIconsForFont(const QFont &font);

};

class TableModelRotation : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {COL_ID=0,COL_NAME,COL_POSITION,COL_NEXT_SONG,COL_REGULAR,COL_ADDTS,COL_DELETE};
    enum {ADD_FAIR=0,ADD_BOTTOM,ADD_NEXT};
    explicit TableModelRotation(QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    void loadData();

    void commitChanges();
    int singerAdd(const QString& name, int positionHint = ADD_BOTTOM);
    void singerMove(int oldPosition, int newPosition, bool skipCommit = false);
    void singerSetName(int singerId, const QString &newName);
    void singerDelete(int singerId);
    bool singerExists(const QString &name);
    bool singerIsRegular(int singerId);
    void singerSetRegular(int singerId, bool isRegular);
    void singerMakeRegular(int singerId);
    std::optional<RotationSinger> getSingerAtPosition(int position);
    uint singerTurnDistance(int singerId);
    void singerDisableRegularTracking(int singerId);
    static bool historySingerExists(const QString &name) ;
    QString getSingerName(int singerId);
    int getSingerId(const QString &name);
    [[nodiscard]] int getSingerPosition(int singerId) const;
    [[nodiscard]] int singerIdAtPosition(int position) const;
    QStringList singers();
    [[nodiscard]] static QStringList historySingers() ;
    [[nodiscard]] static QString nextSongPath(int singerId) ;
    [[nodiscard]] static QString nextSongArtist(int singerId) ;
    [[nodiscard]] static QString nextSongTitle(int singerId) ;
    [[nodiscard]] static QString nextSongArtistTitle(int singerId) ;
    [[nodiscard]] static QString nextSongSongId(int singerId) ;
    [[nodiscard]] int nextSongDurationSecs(int singerId) const;
    int rotationDuration();
    [[nodiscard]] static int nextSongKeyChg(int singerId) ;
    [[nodiscard]] static int nextSongQueueId(int singerId) ;
    void clearRotation();
    [[nodiscard]] int currentSinger() const;
    void setCurrentSinger(int currentSingerId);
    void setRotationTopSingerId(int id);
    bool rotationIsValid();
    [[nodiscard]] static int numSongs(int singerId) ;
    [[nodiscard]] static int numSongsSung(int singerId) ;
    [[nodiscard]] static int numSongsUnsung(int singerId) ;
    QDateTime timeAdded(int singerId);
    void outputRotationDebug();
    void fixSingerPositions();
    void resizeIconsForFont(const QFont &font);
    void setCurRemainSecs(const int secs) { m_remainSecs = secs; }

private:
    std::string m_loggingPrefix{"[RotationModel]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::vector<RotationSinger> m_singers;
    int m_currentSingerId{-1};
    int m_rotationTopSingerId{-1};
    QImage m_iconGreenCircle;
    QImage m_iconYellowCircle;
    int m_curFontHeight{0};
    int m_remainSecs{0};
    Settings m_settings;

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void regularAddNameConflict(QString name);
    void regularLoadNameConflict(QString name);
    void rotationModified();
    void regularsModified();
    void singersMoved(int startRow, int startCol, int endRow, int endCol);

    // QAbstractItemModel interface
public:
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
};

#endif // TABLEMODELROTATION_H
