#ifndef SINGERHISTORYTABLEMODEL_H
#define SINGERHISTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QObject>

struct HistorySong {
    unsigned int id{0};
    unsigned int historySinger{0};
    QString filePath;
    QString artist;
    QString title;
    QString songid;
    int keyChange{0};
    int plays{0};
    QDateTime lastPlayed; // unix time
};

class HistorySongsTableModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::vector<HistorySong> m_songs;
    QString m_currentSinger;
    int m_lastSortColumn{3};
    Qt::SortOrder m_lastSortOrder{Qt::AscendingOrder};
public:
    HistorySongsTableModel();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    void loadSinger(const int historySingerId);
    void loadSinger(const QString historySingerName);
    void saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title,
                  const QString &songid, const int keyChange);
    void saveSong(const QString &singerName, const QString &filePath, const QString &artist, const QString &title,
                  const QString &songid, const int keyChange, int plays, QDateTime lastPlayed);
    void deleteSong(const int historySongId);
    int addSinger(const QString name) const;
    bool songExists(const int historySingerId, const QString &filePath) const;
    int getSingerId(const QString &name) const;
    int getDbSongId(const int historySongId) const;
    std::vector<HistorySong> getSingerSongs(const int historySingerId);
    QString currentSingerName() const { return m_currentSinger; }
    void refresh();
    // QAbstractItemModel interface
public:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // QAbstractItemModel interface
public:
    void sort(int column, Qt::SortOrder order) override;
};

#endif // SINGERHISTORYTABLEMODEL_H
