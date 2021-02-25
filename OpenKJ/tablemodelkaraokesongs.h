#ifndef TABLEMODELKARAOKESONGSNEW_H
#define TABLEMODELKARAOKESONGSNEW_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QImage>

struct KaraokeSong {
    int id{0};
    QString artist;
    QString title;
    QString songid;
    int duration{0};
    QString filename;
    QString path;
    std::string searchString;
    int plays;
    QDateTime lastPlay;
};

class TableModelKaraokeSongs : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum ModelCols{COL_ID=0,COL_ARTIST,COL_TITLE,COL_SONGID,COL_FILENAME,COL_DURATION,COL_PLAYS,COL_LASTPLAY};
    enum {SORT_ARTIST=1,SORT_TITLE=2,SORT_SONGID=3,SORT_DURATION=4};
    enum SearchType{SEARCH_TYPE_ALL=1, SEARCH_TYPE_ARTIST, SEARCH_TYPE_TITLE};
    explicit TableModelKaraokeSongs(QObject *parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void loadData();
    void sort(int column, Qt::SortOrder order) override;
    void search(QString searchString);
    void setSearchType(SearchType type);
    int getIdForPath(const QString &path);
    QString getPath(const int songId);
    void updateSongHistory(const int songid);
    KaraokeSong &getSong(const int songId);


private:
    std::vector<KaraokeSong> m_filteredSongs;
    std::vector<KaraokeSong> m_allSongs;
    QString m_lastSearch;
    Qt::SortOrder m_lastSortOrder{Qt::AscendingOrder};
    int m_lastSortColumn{1};
    int m_curFontHeight;
    QImage m_iconCdg;
    QImage m_iconZip;
    QImage m_iconVid;
    SearchType m_searchType{SearchType::SEARCH_TYPE_ALL};
    void resizeIconsForFont(const QFont &font);
};

#endif // TABLEMODELKARAOKESONGSNEW_H
