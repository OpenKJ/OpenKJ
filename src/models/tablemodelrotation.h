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
#include "../okjtypes.h"

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
    [[maybe_unused]] explicit ItemDelegateRotation(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
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
    okj::RotationSinger InvalidSinger {
        -1,
        "invalid",
        -1,
        false,
        QDateTime(),
        false
    };
    explicit TableModelRotation(QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    [[nodiscard]] const okj::RotationSinger& getSingerAtPosition(int position) const;
    [[nodiscard]] const okj::RotationSinger& getSinger(int singerId) const;
    [[nodiscard]] const okj::RotationSinger& getSingerByName(const QString &name) const;
    [[nodiscard]] int currentSinger() const;
    [[nodiscard]] bool historySingerExists(const QString &name) const;
    [[nodiscard]] QStringList historySingers() const;
    [[nodiscard]] int positionTurnDistance(int position) const;
    [[nodiscard]] int positionWaitTime(int position) const;
    [[nodiscard]] int rotationDuration() const;
    [[nodiscard]] size_t singerCount();
    [[nodiscard]] bool singerExists(const QString &name) const;
    [[nodiscard]] QStringList singers() const;
    [[nodiscard]] int singerTurnDistance(int singerId) const;
    void loadData();
    void commitChanges();
    int singerAdd(const QString& name, int positionHint = ADD_BOTTOM, bool makeRegular = false);
    void singerAddWithSong(const QString &name, int positionHint, int songId, int keyChange, bool makeRegular = false);
    void singerMove(int oldPosition, int newPosition, bool skipCommit = false);
    void singerSetName(int singerId, const QString &newName);
    void singerDelete(int singerId);
    void singerSetRegular(int singerId, bool isRegular);
    void singerMakeRegular(int singerId);
    void singerDisableRegularTracking(int singerId);
    void clearRotation();
    void setCurrentSinger(int currentSingerId);
    void setRotationTopSingerId(int id);
    void outputRotationDebug();
    void fixSingerPositions();
    void resizeIconsForFont(const QFont &font);
    void setCurRemainSecs(const int secs) { m_remainSecs = secs; }

private:
    std::string m_loggingPrefix{"[RotationModel]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::vector<okj::RotationSinger> m_singers;
    int m_currentSingerId{-1};
    int m_rotationTopSingerId{-1};
    QImage m_iconGreenCircle;
    QImage m_iconYellowCircle;
    int m_curFontHeight{0};
    int m_remainSecs{0};
    Settings m_settings;
    [[nodiscard]] QVariant getTooltipData(const QModelIndex &index) const;
    [[nodiscard]] QVariant getDisplayData(const QModelIndex &index) const;
    [[nodiscard]] QVariant getColumnSizeHint(const QModelIndex &index) const;
    [[nodiscard]] QVariant getDecorationRole(const QModelIndex &index) const;
    [[nodiscard]] QVariant getBackgroundRole(const QModelIndex &index) const;
    [[nodiscard]] static QString getWaitTimeString(int totalWaitDuration);


public slots:
    void onRotationLoaded(const QList<okj::RotationSinger> &singers);
    void onSingerAdded(int singerId, const QString &name, int positionHint);

signals:
    void songDroppedOnSinger(int singerId, int songId, int dropRow);
    void rotationModified();
    void singersMoved(int startRow, int startCol, int endRow, int endCol);
    void newSingerAdded(int position);

    void requestLoadRotation();
    void requestCommitRotation(const QList<okj::RotationSinger> &singers);
    void requestAddSinger(const QString &name, int positionHint, bool makeRegular);
    void requestAddSingerWithSong(const QString &name, int positionHint, int songId, int keyChange, bool makeRegular);
    void requestRenameSinger(int singerId, const QString &newName);
    void requestDeleteSinger(int singerId);
    void requestSetSingerRegular(int singerId, bool isRegular);
    void requestClearRotation();

};

#endif // TABLEMODELROTATION_H
