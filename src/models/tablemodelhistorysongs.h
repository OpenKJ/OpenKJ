#ifndef SINGERHISTORYTABLEMODEL_H
#define SINGERHISTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QObject>
#include "tablemodelkaraokesongs.h"
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class TableModelHistorySongs : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::string m_loggingPrefix{"[HistorySongsModel]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::vector<okj::HistorySong> m_songs;
    QString m_currentSinger;
    int m_lastSortColumn{3};
    Qt::SortOrder m_lastSortOrder{Qt::AscendingOrder};
    TableModelKaraokeSongs &m_karaokeSongsModel;
    Settings m_settings;
    QFont m_itemFont;
    QFontMetrics m_itemFontMetrics{m_settings.applicationFont()};
    int m_itemHeight{20};
    QFont m_headerFont;

    QVariant getSizeHint(int section) const;
    QString getColumnName(int section) const;

public slots:
    void setFont(const QFont &font);

public:
    enum {
        SONG_ID=0,
        SINGER_ID,
        PATH,
        ARTIST,
        TITLE,
        SONGID,
        KEY_CHANGE,
        SUNG_COUNT,
        LAST_SUNG
    };
    explicit TableModelHistorySongs(TableModelKaraokeSongs &songsModel);
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    void loadSinger(int historySingerId);
    void loadSinger(const QString &historySingerName);
    void saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title,
                  const QString &songid, int keyChange);
    void saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title,
                  const QString &songid, int keyChange, int plays, const QDateTime& lastPlayed);
    void deleteSong(int historySongId);
    [[nodiscard]] int addSinger(const QString &name) const;
    [[nodiscard]] bool songExists(int historySingerId, const QString &filePath) const;
    [[nodiscard]] int getSingerId(const QString &name) const;
    [[nodiscard]] std::vector<okj::HistorySong> getSingerSongs(int historySingerId);
    [[nodiscard]] QString currentSingerName() const { return m_currentSinger; }
    void refresh();
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void sort(int column, Qt::SortOrder order) override;
    [[nodiscard]] QVariant getDisplayData(const QModelIndex &index) const;
    [[nodiscard]] static QVariant getTextAlignment(const QModelIndex &index) ;

};

#endif // SINGERHISTORYTABLEMODEL_H
