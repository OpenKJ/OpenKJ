#include "tablemodelkaraokesongs.h"

#include <QSqlQuery>
#include <QDebug>
#include <QPainter>
#include <QSvgRenderer>
#include <QMimeData>
#include "settings.h"

extern Settings settings;

TableModelKaraokeSongs::TableModelKaraokeSongs(QObject *parent)
        : QAbstractTableModel(parent) {
    resizeIconsForFont(settings.applicationFont());
}

QVariant TableModelKaraokeSongs::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case TableModelKaraokeSongs::COL_ID:
                return "ID";
            case TableModelKaraokeSongs::COL_ARTIST:
                return "Artist";
            case TableModelKaraokeSongs::COL_TITLE:
                return "Title";
            case TableModelKaraokeSongs::COL_SONGID:
                return "SongID";
            case TableModelKaraokeSongs::COL_FILENAME:
                return "Filename";
            case TableModelKaraokeSongs::COL_DURATION:
                return "Duration";
            case TableModelKaraokeSongs::COL_PLAYS:
                return "Plays";
            case TableModelKaraokeSongs::COL_LASTPLAY:
                return "Last Played";
            default:
                return QVariant();
        }
    }
    return QVariant();
}

int TableModelKaraokeSongs::rowCount([[maybe_unused]]const QModelIndex &parent) const {
    return m_filteredSongs.size();
}

int TableModelKaraokeSongs::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    return 8;
}

QVariant TableModelKaraokeSongs::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();
    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case COL_DURATION:
            case COL_PLAYS:
            case COL_LASTPLAY:
                return Qt::AlignRight + Qt::AlignVCenter;
            default:
                return Qt::AlignLeft + Qt::AlignVCenter;
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == COL_SONGID) {
            if (m_filteredSongs.at(index.row()).get().path.endsWith("cdg", Qt::CaseInsensitive))
                return m_iconCdg;
            else if (m_filteredSongs.at(index.row()).get().path.endsWith("zip", Qt::CaseInsensitive))
                return m_iconZip;
            else
                return m_iconVid;
        }
    } else if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case TableModelKaraokeSongs::COL_ID:
                return m_filteredSongs.at(index.row()).get().id;
            case TableModelKaraokeSongs::COL_ARTIST:
                return m_filteredSongs.at(index.row()).get().artist;
            case TableModelKaraokeSongs::COL_TITLE:
                return m_filteredSongs.at(index.row()).get().title;
            case TableModelKaraokeSongs::COL_SONGID:
                return m_filteredSongs.at(index.row()).get().songid;
            case TableModelKaraokeSongs::COL_FILENAME:
                return m_filteredSongs.at(index.row()).get().filename;
            case TableModelKaraokeSongs::COL_DURATION:
                if (m_filteredSongs.at(index.row()).get().duration < 1)
                    return QVariant();
                return QTime(0, 0, 0, 0).addSecs(m_filteredSongs.at(index.row()).get().duration / 1000).toString(
                        "m:ss");
            case TableModelKaraokeSongs::COL_PLAYS:
                return m_filteredSongs.at(index.row()).get().plays;
            case TableModelKaraokeSongs::COL_LASTPLAY:
                QLocale locale;
                return m_filteredSongs.at(index.row()).get().lastPlay.toString(
                        locale.dateTimeFormat(QLocale::ShortFormat));
        }
    }
    return QVariant();
}

void TableModelKaraokeSongs::loadData() {
    emit layoutAboutToBeChanged();
    m_allSongs.clear();
    m_filteredSongs.clear();
    QSqlQuery query;
    query.exec("SELECT songid,artist,title,discid,duration,filename,path,searchstring,plays,lastplay "
               "FROM dbsongs WHERE discid != '!!BAD!!' AND discid != '!!DROPPED!!'");
    if (query.size() > 0)
        m_filteredSongs.reserve(query.size());
    while (query.next()) {
        m_allSongs.emplace_back(KaraokeSong{
                query.value(0).toInt(),
                query.value(1).toString(),
                query.value(2).toString(),
                query.value(3).toString(),
                query.value(4).toInt(),
                query.value(5).toString(),
                query.value(6).toString(),
                query.value(7).toString().toLower().toStdString(),
                query.value(8).toInt(),
                query.value(9).toDateTime()
        });
    }
    qInfo() << "Loaded " << m_filteredSongs.size() << " karaoke songs from database.";
    search(m_lastSearch);
    emit layoutChanged();
}

void TableModelKaraokeSongs::search(const QString &searchString) {
    m_lastSearch = searchString;
    emit layoutAboutToBeChanged();
    std::vector<std::string> searchTerms;
    std::string s = searchString.toLower().toStdString();
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(' ', pos)) != std::string::npos) {
        searchTerms.emplace_back(s.substr(prev_pos, pos - prev_pos));
        prev_pos = ++pos;
    }
    searchTerms.emplace_back(s.substr(prev_pos, pos - prev_pos));
    m_filteredSongs.clear();
    m_filteredSongs.reserve(m_allSongs.size());
    std::for_each(m_allSongs.begin(), m_allSongs.end(), [&](auto &song) {
        std::string searchString;
        switch (m_searchType) {
            case TableModelKaraokeSongs::SEARCH_TYPE_ALL:
                searchString = song.searchString;
                break;
            case TableModelKaraokeSongs::SEARCH_TYPE_ARTIST:
                searchString = song.artist.toLower().toStdString();
                break;
            case TableModelKaraokeSongs::SEARCH_TYPE_TITLE:
                searchString = song.title.toLower().toStdString();
                break;
        }
        bool match{true};
        for (const auto &term : searchTerms) {
            if (searchString.find(term) == std::string::npos)
                match = false;
        }
        if (match)
            m_filteredSongs.template emplace_back(song);
    });
    m_filteredSongs.shrink_to_fit();
    sort(m_lastSortColumn, m_lastSortOrder);
    emit layoutChanged();
}

void TableModelKaraokeSongs::setSearchType(TableModelKaraokeSongs::SearchType type) {
    if (m_searchType == type)
        return;
    m_searchType = type;
    search(m_lastSearch);
}

int TableModelKaraokeSongs::getIdForPath(const QString &path) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&](KaraokeSong &song) {
        return (song.path == path);
    });
    if (it == m_allSongs.end())
        return -1;
    return it->id;
}

QString TableModelKaraokeSongs::getPath(const int songId) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](KaraokeSong &song) {
        return (song.id == songId);
    });
    return it->path;
}

void TableModelKaraokeSongs::updateSongHistory(const int songId) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](KaraokeSong &song) {
        if (song.id == songId)
            return true;
        return false;
    });
    if (it != m_allSongs.end()) {
        it->plays++;
        it->lastPlay = QDateTime::currentDateTime();
    }

    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(), [&songId](auto song) {
        if (song.get().id == songId)
            return true;
        return false;
    });
    if (it2 != m_filteredSongs.end()) {
        int row = std::distance(m_filteredSongs.begin(), it2);
        emit dataChanged(this->index(row, COL_PLAYS), this->index(row, COL_LASTPLAY), QVector<int>(Qt::DisplayRole));
    }

    QSqlQuery query;
    query.prepare("UPDATE dbSongs set plays = plays + :incVal, lastplay = :curTs WHERE songid = :songid");
    query.bindValue(":curTs", QDateTime::currentDateTime());
    query.bindValue(":songid", songId);
    query.bindValue(":incVal", 1);
    query.exec();
}

KaraokeSong &TableModelKaraokeSongs::getSong(const int songId) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](KaraokeSong &song) {
        return (song.id == songId);
    });
    return *it;
}

void TableModelKaraokeSongs::resizeIconsForFont(const QFont &font) {
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconVid = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconZip = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconCdg = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconVid.fill(Qt::transparent);
    m_iconZip.fill(Qt::transparent);
    m_iconCdg.fill(Qt::transparent);
    QPainter painterVid(&m_iconVid);
    QPainter painterZip(&m_iconZip);
    QPainter painterCdg(&m_iconCdg);
    QSvgRenderer svgrndrVid(thm + "mimetypes/22/video-mp4.svg");
    QSvgRenderer svgrndrZip(thm + "mimetypes/22/application-zip.svg");
    QSvgRenderer svgrndrCdg(thm + "mimetypes/22/application-x-cda.svg");
    svgrndrVid.render(&painterVid);
    svgrndrZip.render(&painterZip);
    svgrndrCdg.render(&painterCdg);
}


QMimeData *TableModelKaraokeSongs::mimeData(const QModelIndexList &indexes) const {
    auto *mimeData = new QMimeData();
    mimeData->setData("integer/songid", data(indexes.at(0).sibling(indexes.at(0).row(), COL_ID), Qt::DisplayRole).toByteArray().data());
    return mimeData;
}

Qt::ItemFlags TableModelKaraokeSongs::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}


void TableModelKaraokeSongs::sort(int column, Qt::SortOrder order) {
    m_lastSortColumn = column;
    m_lastSortOrder = order;
    emit layoutAboutToBeChanged();
    if (order == Qt::AscendingOrder) {
        std::sort(m_filteredSongs.begin(), m_filteredSongs.end(), [&column](KaraokeSong &a, KaraokeSong &b) {
            switch (column) {
                case COL_ARTIST:
                    return (a.artist.toLower() < b.artist.toLower());
                case COL_TITLE:
                    return (a.title.toLower() < b.title.toLower());
                case COL_SONGID:
                    return (a.songid.toLower() < b.songid.toLower());
                case COL_FILENAME:
                    return (a.filename.toLower() < b.filename.toLower());
                case COL_DURATION:
                    return (a.duration < b.duration);
                case COL_PLAYS:
                    return (a.plays < b.plays);
                case COL_LASTPLAY:
                    return (a.lastPlay < b.lastPlay);

                default:
                    return (a.id < b.id);
            }
        });
    } else {
        std::sort(m_filteredSongs.rbegin(), m_filteredSongs.rend(), [&column](KaraokeSong &a, KaraokeSong &b) {
            switch (column) {
                case COL_ARTIST:
                    return (a.artist.toLower() < b.artist.toLower());
                case COL_TITLE:
                    return (a.title.toLower() < b.title.toLower());
                case COL_SONGID:
                    return (a.songid.toLower() < b.songid.toLower());
                case COL_FILENAME:
                    return (a.filename.toLower() < b.filename.toLower());
                case COL_DURATION:
                    return (a.duration < b.duration);
                case COL_PLAYS:
                    return (a.plays < b.plays);
                case COL_LASTPLAY:
                    return (a.lastPlay < b.lastPlay);
                default:
                    return (a.id < b.id);
            }
        });
    }
    emit layoutChanged();
}

void TableModelKaraokeSongs::setSongDuration(QString &path, int duration) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&path](auto &song) {
        return (song.path == path);
    });
    if (it == m_allSongs.end())
        return;
    it->duration = duration;
    int songId = it->id;
    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(), [&songId](auto &song) {
        return (song.get().id == songId);
    });
    if (it2 != m_filteredSongs.end()) {
        int row = std::distance(m_filteredSongs.begin(), it2);
        emit dataChanged(this->index(row, COL_DURATION), this->index(row, COL_DURATION), QVector<int>(Qt::DisplayRole));
    }
}
