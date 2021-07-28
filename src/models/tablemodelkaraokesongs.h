#ifndef TABLEMODELKARAOKESONGS_H
#define TABLEMODELKARAOKESONGS_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QImage>
#include <memory>
#include <QTimer>

struct KaraokeSong {
    int id{0};
    QString artist;
    QString artistL;
    QString title;
    QString titleL;
    QString songid;
    QString songidL;
    int duration{0};
    QString filename;
    QString path;
    QString searchString;
    int plays;
    QDateTime lastPlay;
    bool bad{false};
    bool dropped{false};
};

class TableModelKaraokeSongs : public QAbstractTableModel {
Q_OBJECT

public:
    enum ModelCols {
        COL_ID=0,
        COL_ARTIST,
        COL_TITLE,
        COL_SONGID,
        COL_FILENAME,
        COL_DURATION,
        COL_PLAYS,
        COL_LASTPLAY
    };
    enum SearchType {
        SEARCH_TYPE_ALL=1,
        SEARCH_TYPE_ARTIST,
        SEARCH_TYPE_TITLE
    };
    enum DeleteStatus {
        DELETE_OK,
        DELETE_FAIL,
        DELETE_CDG_AUDIO_FAIL
    };

    explicit TableModelKaraokeSongs(QObject *parent = nullptr);
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    void loadData();
    void sort(int column, Qt::SortOrder order) override;
    void search(const QString &searchString);
    void setSearchType(SearchType type);
    int getIdForPath(const QString &path);
    QString getPath(int songId);
    void updateSongHistory(int songId);
    KaraokeSong &getSong(int songId);
    void markSongBad(QString path);
    DeleteStatus removeBadSong(QString path);
    static QString findCdgAudioFile(const QString& path);
    int addSong(KaraokeSong song);


private:
    std::vector<std::shared_ptr<KaraokeSong>> m_filteredSongs;
    std::vector< std::shared_ptr<KaraokeSong> > m_allSongs;
    QString m_lastSearch;
    Qt::SortOrder m_lastSortOrder{Qt::AscendingOrder};
    int m_lastSortColumn{1};
    int m_curFontHeight{0};
    QImage m_iconCdg;
    QImage m_iconZip;
    QImage m_iconVid;
    SearchType m_searchType{SearchType::SEARCH_TYPE_ALL};

    void resizeIconsForFont(const QFont &font);
    void searchExec();
    QTimer searchTimer{this};

public slots:

    void setSongDuration(const QString &path, unsigned int duration);
};

#endif // TABLEMODELKARAOKESONGS_H
