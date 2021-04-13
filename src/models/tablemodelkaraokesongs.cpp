#include "tablemodelkaraokesongs.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QSvgRenderer>
#include <QMimeData>
#include <QApplication>
#include <execution>
#include "settings.h"

extern Settings settings;

TableModelKaraokeSongs::TableModelKaraokeSongs(QObject *parent)
        : QAbstractTableModel(parent) {
    resizeIconsForFont(settings.applicationFont());
    connect(&searchTimer, &QTimer::timeout, this, &TableModelKaraokeSongs::searchExec);
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
    return (int) m_filteredSongs.size();
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
            if (m_filteredSongs.at(index.row())->path.endsWith("cdg", Qt::CaseInsensitive))
                return m_iconCdg;
            else if (m_filteredSongs.at(index.row())->path.endsWith("zip", Qt::CaseInsensitive))
                return m_iconZip;
            else
                return m_iconVid;
        }
    } else if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case TableModelKaraokeSongs::COL_ID:
                return m_filteredSongs.at(index.row())->id;
            case TableModelKaraokeSongs::COL_ARTIST:
                return m_filteredSongs.at(index.row())->artist;
            case TableModelKaraokeSongs::COL_TITLE:
                return m_filteredSongs.at(index.row())->title;
            case TableModelKaraokeSongs::COL_SONGID:
                return m_filteredSongs.at(index.row())->songid;
            case TableModelKaraokeSongs::COL_FILENAME:
                return m_filteredSongs.at(index.row())->filename;
            case TableModelKaraokeSongs::COL_DURATION:
                if (m_filteredSongs.at(index.row())->duration < 1)
                    return QVariant();
                return QTime(0, 0, 0, 0).addSecs(m_filteredSongs.at(index.row())->duration / 1000).toString(
                        "m:ss");
            case TableModelKaraokeSongs::COL_PLAYS:
                return m_filteredSongs.at(index.row())->plays;
            case TableModelKaraokeSongs::COL_LASTPLAY:
                QLocale locale;
                return m_filteredSongs.at(index.row())->lastPlay.toString(
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
               "FROM dbsongs WHERE discid != '!!BAD!!'");
    if (query.size() > 0)
        m_filteredSongs.reserve(query.size());
    while (query.next()) {
        auto song = m_allSongs.emplace_back(std::make_shared<KaraokeSong>(KaraokeSong{
                query.value(0).toInt(),
                query.value(1).toString(),
                query.value(1).toString().toLower(),
                query.value(2).toString(),
                query.value(2).toString().toLower(),
                query.value(3).toString(),
                query.value(3).toString().toLower(),
                query.value(4).toInt(),
                query.value(5).toString(),
                query.value(6).toString(),
                query.value(7).toString().replace('&', " and ").toLower(),
                query.value(8).toInt(),
                query.value(9).toDateTime()
        }));
    }
    qInfo() << "Loaded " << m_filteredSongs.size() << " karaoke songs from database.";
    search(m_lastSearch);
    emit layoutChanged();
}

void TableModelKaraokeSongs::search(const QString &searchString) {
    m_lastSearch = searchString.toLower();
    m_lastSearch.replace(',', ' ');
    m_lastSearch.replace('&', " and ");
    if (settings.ignoreAposInSearch())
        m_lastSearch.replace('\'', ' ');
    if (searchTimer.isActive())
        searchTimer.stop();
    searchTimer.start(100);
}

void TableModelKaraokeSongs::searchExec() {
    searchTimer.stop();
    emit layoutAboutToBeChanged();
    std::vector<std::string> searchTerms;
    std::string s = m_lastSearch.toLower().toStdString();
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(' ', pos)) != std::string::npos) {
        searchTerms.emplace_back(s.substr(prev_pos, pos - prev_pos));
        prev_pos = ++pos;
    }
    searchTerms.emplace_back(s.substr(prev_pos, pos - prev_pos));
    m_filteredSongs.clear();
    m_filteredSongs.reserve(m_allSongs.size());
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    auto needles = m_lastSearch.split(' ', QString::SplitBehavior::SkipEmptyParts);
#else
    auto needles = m_lastSearch.split(' ', Qt::SplitBehavior(Qt::SkipEmptyParts));
#endif
    std::for_each(m_allSongs.begin(), m_allSongs.end(), [&](const std::shared_ptr<KaraokeSong> &song) {
        if (song->songid.contains("!!DROPPED!!"))
            return;
        QString haystack;
        switch (m_searchType) {
            case TableModelKaraokeSongs::SEARCH_TYPE_ALL: {
                haystack = song->searchString;
                break;
            }
            case TableModelKaraokeSongs::SEARCH_TYPE_ARTIST: {
                haystack = song->artistL.replace('&', " and ");
                break;
            }
            case TableModelKaraokeSongs::SEARCH_TYPE_TITLE: {
                haystack = song->titleL.replace('&', " and ");
                break;
            }
        }
        if (settings.ignoreAposInSearch())
            haystack.remove('\'');
        bool match{true};
        for (const auto &needle : needles) {
            if (!haystack.contains(needle)) {
                match = false;
                break;
            }
        }
        if (match)
            m_filteredSongs.emplace_back(song);
    });
    m_filteredSongs.shrink_to_fit();
    emit layoutChanged();
}

void TableModelKaraokeSongs::setSearchType(TableModelKaraokeSongs::SearchType type) {
    if (m_searchType == type)
        return;
    m_searchType = type;
    search(m_lastSearch);
}

int TableModelKaraokeSongs::getIdForPath(const QString &path) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&](const std::shared_ptr<KaraokeSong> &song) {
        return (song->path == path);
    });
    if (it == m_allSongs.end())
        return -1;
    return it->get()->id;
}

QString TableModelKaraokeSongs::getPath(const int songId) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<KaraokeSong> &song) {
        return (song->id == songId);
    });
    return it->get()->path;
}

void TableModelKaraokeSongs::updateSongHistory(const int songId) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<KaraokeSong> &song) {
        if (song->id == songId)
            return true;
        return false;
    });
    if (it != m_allSongs.end()) {
        it->get()->plays++;
        it->get()->lastPlay = QDateTime::currentDateTime();
    }

    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                       [&songId](const std::shared_ptr<KaraokeSong> &song) {
                           if (song->id == songId)
                               return true;
                           return false;
                       });
    if (it2 != m_filteredSongs.end()) {
        int row = (int) std::distance(m_filteredSongs.begin(), it2);
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
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<KaraokeSong> &song) {
        return (song->id == songId);
    });
    return **it;
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
    mimeData->setData("integer/songid",
                      data(indexes.at(0).sibling(indexes.at(0).row(), COL_ID), Qt::DisplayRole).toByteArray().data());
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

    auto sortLambda = [&column](const std::shared_ptr<KaraokeSong> &a, const std::shared_ptr<KaraokeSong> &b) -> bool {
        switch (column) {
            case COL_ARTIST:
                if (a->artistL == b->artistL) {
                    if (a->titleL == b->titleL) {
                        return (a->songidL < b->songidL);
                    }
                    return (a->titleL < b->titleL);
                }
                return (a->artistL < b->artistL);
            case COL_TITLE:
                if (a->titleL == b->titleL) {
                    if (a->artistL == b->artistL) {
                        return (a->songidL < b->songidL);
                    }
                    return (a->artistL < b->artistL);
                }
                return (a->titleL < b->titleL);
            case COL_SONGID:
                return (a->songidL < b->songidL);
            case COL_FILENAME:
                return (a->filename.toLower() < b->filename.toLower());
            case COL_DURATION:
                return (a->duration < b->duration);
            case COL_PLAYS:
                return (a->plays < b->plays);
            case COL_LASTPLAY:
                return (a->lastPlay < b->lastPlay);

            default:
                return (a->id < b->id);
        }
    };

    QApplication::setOverrideCursor(Qt::BusyCursor);
    if (order == Qt::AscendingOrder) {
        std::sort(std::execution::par, m_allSongs.begin(), m_allSongs.end(), sortLambda);
    } else {
        std::sort(std::execution::par, m_allSongs.rbegin(), m_allSongs.rend(), sortLambda);
    }
    QApplication::restoreOverrideCursor();
    search(m_lastSearch);
}

void TableModelKaraokeSongs::setSongDuration(QString &path, int duration) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&path](const std::shared_ptr<KaraokeSong> &song) {
        return (song->path == path);
    });
    if (it == m_allSongs.end())
        return;
    it->get()->duration = duration;
    int songId = it->get()->id;
    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                       [&songId](const std::shared_ptr<KaraokeSong> &song) {
                           return (song->id == songId);
                       });
    if (it2 != m_filteredSongs.end()) {
        int row = (int) std::distance(m_filteredSongs.begin(), it2);
        emit dataChanged(this->index(row, COL_DURATION), this->index(row, COL_DURATION), QVector<int>(Qt::DisplayRole));
    }
}

void TableModelKaraokeSongs::markSongBad(QString path) {
    QSqlQuery query;
    query.prepare("UPDATE dbsongs SET discid='!!BAD!!' WHERE path == :path");
    query.bindValue(":path", path);
    query.exec();

    emit layoutAboutToBeChanged();
    auto newFilteredEnd = std::remove_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                                         [&path](const std::shared_ptr<KaraokeSong> &song) {
                                             return (song->path == path);
                                         });
    m_filteredSongs.erase(newFilteredEnd, m_filteredSongs.end());
    emit layoutChanged();

    auto newAllSongsEnd = std::remove_if(m_allSongs.begin(), m_allSongs.end(),
                                         [&path](const std::shared_ptr<KaraokeSong> &song) {
                                             return (song->path == path);
                                         });
    m_allSongs.erase(newAllSongsEnd, m_allSongs.end());

}

TableModelKaraokeSongs::DeleteStatus TableModelKaraokeSongs::removeBadSong(QString path) {
    bool isCdg = false;
    if (QFileInfo(path).suffix().toLower() == "cdg")
        isCdg = true;
    QString mediaFile;
    if (isCdg)
        mediaFile = findCdgAudioFile(path);
    QFile file(path);
    if (file.remove()) {
        QSqlQuery query;
        query.prepare("DELETE FROM dbsongs WHERE path == :path");
        query.bindValue(":path", path);
        query.exec();

        emit layoutAboutToBeChanged();
        auto newFilteredEnd = std::remove_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                                             [&path](const std::shared_ptr<KaraokeSong> &song) {
                                                 return (song->path == path);
                                             });
        m_filteredSongs.erase(newFilteredEnd, m_filteredSongs.end());

        emit layoutChanged();
        auto newAllSongsEnd = std::remove_if(m_allSongs.begin(), m_allSongs.end(),
                                             [&path](const std::shared_ptr<KaraokeSong> &song) {
                                                 return (song->path == path);
                                             });
        m_allSongs.erase(newAllSongsEnd, m_allSongs.end());

        if (isCdg) {
            if (!QFile::remove(mediaFile)) {
                return DELETE_CDG_AUDIO_FAIL;
            }
        }
    } else {
        return DELETE_FAIL;
    }
    return DELETE_OK;
}

QString TableModelKaraokeSongs::findCdgAudioFile(const QString &path) {
    qInfo() << "findMatchingAudioFile(" << path << ") called";
    QStringList audioExtensions;
    audioExtensions.append("mp3");
    audioExtensions.append("wav");
    audioExtensions.append("ogg");
    audioExtensions.append("mov");
    audioExtensions.append("flac");
    QFileInfo cdgInfo(path);
    QDir srcDir = cdgInfo.absoluteDir();
    QDirIterator it(srcDir);
    while (it.hasNext()) {
        it.next();
        if (it.fileInfo().completeBaseName() != cdgInfo.completeBaseName())
            continue;
        if (it.fileInfo().suffix().toLower() == "cdg")
            continue;
        QString ext;
                foreach (ext, audioExtensions) {
                if (it.fileInfo().suffix().toLower() == ext) {
                    qInfo() << "findMatchingAudioFile found match: " << it.filePath();
                    return it.filePath();
                }
            }
    }
    qInfo() << "findMatchingAudioFile found no matches";
    return QString();
}

int TableModelKaraokeSongs::addSong(KaraokeSong song) {
    qInfo() << "TableModelKaraokeSongs::addSong() called";
    if (int songId = getIdForPath(song.path); songId > -1) {
        qInfo() << "addSong() - Song at path already exists in db:\n\t" << song.path;
        return songId;
    }
    QSqlQuery query;
    query.prepare(
            "INSERT INTO dbSongs (discid,artist,title,path,duration,filename,searchstring) VALUES(:songid, :artist, :title, :path, :duration, :filename, :searchString)");
    query.bindValue(":songid", song.songid);
    query.bindValue(":artist", song.artist);
    query.bindValue(":title", song.title);
    query.bindValue(":path", song.path);
    query.bindValue(":duration", song.duration);
    query.bindValue(":filename", song.filename);
    query.bindValue(":searchString", song.searchString);
    query.exec();
    qInfo() << query.lastError();
    if (query.lastInsertId().isValid()) {
        int lastInsertId = query.lastInsertId().toInt();
        song.id = lastInsertId;
        m_allSongs.push_back(std::make_shared<KaraokeSong>(song));
        search(m_lastSearch);
        return lastInsertId;
    } else {
        qInfo() << "Error while inserting song into DB";
        return -1;
    }
}
