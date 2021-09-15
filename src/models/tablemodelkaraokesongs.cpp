#include "tablemodelkaraokesongs.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QSvgRenderer>
#include <QMimeData>
#include <array>

std::ostream & operator<<(std::ostream& os, const QString& s);

TableModelKaraokeSongs::TableModelKaraokeSongs(QObject *parent)
        : QAbstractTableModel(parent) {
    m_logger = spdlog::get("logger");
    resizeIconsForFont(m_settings.applicationFont());
    connect(&searchTimer, &QTimer::timeout, this, &TableModelKaraokeSongs::searchExec);
}

QVariant TableModelKaraokeSongs::headerData(int section, Qt::Orientation orientation, int role) const {
    switch (role) {
        case Qt::FontRole:
            return m_headerFont;
        case Qt::DisplayRole:
            if (orientation == Qt::Horizontal)
                return getColumnName(section);
            break;
        case Qt::SizeHintRole:
            if (orientation == Qt::Horizontal)
                return getColumnSizeHint(section);
        default:
            return {};
    }
    return {};
}

QVariant TableModelKaraokeSongs::getColumnSizeHint(int section) const {
    switch (section) {
        case COL_ID:
            return QSize(m_itemFontMetrics.size(Qt::TextSingleLine, "_ID_").width(), m_itemHeight);

        case COL_DURATION:
            return QSize(m_itemFontMetrics.size(Qt::TextSingleLine, "_Time_").width(), m_itemHeight);
        case COL_PLAYS:
            return QSize(m_itemFontMetrics.size(Qt::TextSingleLine, "_Plays_").width(), m_itemHeight);
        case COL_LASTPLAY:
            return QSize(m_itemFontMetrics.size(Qt::TextSingleLine, "_10:00 10/00/00_PM").width(), m_itemHeight);
        case COL_SONGID:
            return QSize(m_itemFontMetrics.size(Qt::TextSingleLine, "XXXX0000000-01-00").width(), m_itemHeight);
        case COL_ARTIST:
        case COL_TITLE:
        case COL_FILENAME:
        default:
            return {};
    }
}

QVariant TableModelKaraokeSongs::getColumnName(int section) {
    switch (section) {
        case COL_ID:
            return "ID";
        case COL_ARTIST:
            return "Artist";
        case COL_TITLE:
            return "Title";
        case COL_SONGID:
            return "SongID";
        case COL_FILENAME:
            return "Filename";
        case COL_DURATION:
            return "Time";
        case COL_PLAYS:
            return "Plays";
        case COL_LASTPLAY:
            return "Last Played";
        default:
            return {};
    }
}

int TableModelKaraokeSongs::rowCount([[maybe_unused]]const QModelIndex &parent) const {
    return (int) m_filteredSongs.size();
}

int TableModelKaraokeSongs::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    return 8;
}

QVariant TableModelKaraokeSongs::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return {};
    if (role == Qt::FontRole)
        return m_itemFont;
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
        return getItemDisplayData(index);
    }
    if (role == Qt::SizeHintRole)
        return m_itemFontMetrics.size(Qt::TextSingleLine, getItemDisplayData(index).toString());
    return {};
}

QVariant TableModelKaraokeSongs::getItemDisplayData(const QModelIndex &index) const {
    switch (index.column()) {
        case COL_ID:
            return m_filteredSongs.at(index.row())->id;
        case COL_ARTIST:
            return m_filteredSongs.at(index.row())->artist;
        case COL_TITLE:
            return m_filteredSongs.at(index.row())->title;
        case COL_SONGID:
            return m_filteredSongs.at(index.row())->songid;
        case COL_FILENAME:
            return m_filteredSongs.at(index.row())->filename;
        case COL_DURATION:
            if (m_filteredSongs.at(index.row())->duration < 1)
                return {};
            return QTime(0, 0, 0, 0).addSecs(m_filteredSongs.at(index.row())->duration / 1000).toString(
                    "m:ss");
        case COL_PLAYS:
            return m_filteredSongs.at(index.row())->plays;
        case COL_LASTPLAY: {
            QLocale locale;
            return m_filteredSongs.at(index.row())->lastPlay.toString(
                    locale.dateTimeFormat(QLocale::ShortFormat));
        }
        default:
            return {};
    }
}

void TableModelKaraokeSongs::loadData() {
    emit layoutAboutToBeChanged();
    m_allSongs.clear();
    m_filteredSongs.clear();
    QSqlQuery query;
    query.exec("SELECT songid,artist,title,discid,duration,filename,path,searchstring,plays,lastplay "
               "FROM dbsongs");
    if (query.size() > 0)
        m_filteredSongs.reserve(query.size());
    while (query.next()) {
        auto song = m_allSongs.emplace_back(std::make_shared<okj::KaraokeSong>(okj::KaraokeSong{
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
                query.value(9).toDateTime(),
                (query.value(3).toString() == "!!BAD!!"),
                (query.value(3).toString() == "!!DROPPED!!")
        }));
    }
    m_logger->info("{} Loaded {} karaoke songs from the db on disk", m_loggingPrefix, m_filteredSongs.size());
    search(m_lastSearch);
    emit layoutChanged();
}

void TableModelKaraokeSongs::search(const QString &searchString) {
    m_lastSearch = searchString.toLower();
    m_lastSearch.replace(',', ' ');
    m_lastSearch.replace('&', " and ");
    if (m_settings.ignoreAposInSearch())
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
    for (const auto &song : m_allSongs) {
        if (song->dropped)
            continue;
        if (song->bad)
            continue;
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
        if (m_settings.ignoreAposInSearch())
            haystack.remove('\'');
        bool match{true};
        for (const auto &needle : needles) {
            if (!haystack.contains(needle)) {
                match = false;
                continue;
            }
        }
        if (match)
            m_filteredSongs.emplace_back(song);
    }
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
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&](const std::shared_ptr<okj::KaraokeSong> &song) {
        return (song->path == path);
    });
    if (it == m_allSongs.end())
        return -1;
    return it->get()->id;
}

QString TableModelKaraokeSongs::getPath(const int songId) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<okj::KaraokeSong> &song) {
        return (song->id == songId);
    });
    return it->get()->path;
}

void TableModelKaraokeSongs::updateSongHistory(const int songId) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<okj::KaraokeSong> &song) {
        if (song->id == songId)
            return true;
        return false;
    });
    if (it != m_allSongs.end()) {
        it->get()->plays++;
        it->get()->lastPlay = QDateTime::currentDateTime();
    }

    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                       [&songId](const std::shared_ptr<okj::KaraokeSong> &song) {
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

okj::KaraokeSong &TableModelKaraokeSongs::getSong(const int songId) {
    auto it = std::find_if(m_allSongs.begin(), m_allSongs.end(), [&songId](const std::shared_ptr<okj::KaraokeSong> &song) {
        return (song->id == songId);
    });
    return **it;
}

void TableModelKaraokeSongs::resizeIconsForFont(const QFont &font) {
    m_logger->trace("{} resizeIconsForFont called with font: {} size: {}", m_loggingPrefix, font.toString().toStdString(), QFontMetrics(font).height());
    m_itemFont = m_settings.applicationFont();
    m_headerFont = m_settings.applicationFont();
    m_headerFont.setBold(true);
    m_itemFontMetrics = QFontMetrics(m_itemFont);
    m_headerFontMetrics = QFontMetrics(m_headerFont);
    m_itemHeight = m_itemFontMetrics.height() + 6;

    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
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
    auto sortLambda = [&column](const std::shared_ptr<okj::KaraokeSong> &a, const std::shared_ptr<okj::KaraokeSong> &b) -> bool {
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
        std::sort(m_allSongs.begin(), m_allSongs.end(), sortLambda);
    } else {
        std::sort(m_allSongs.rbegin(), m_allSongs.rend(), sortLambda);
    }
    QApplication::restoreOverrideCursor();
    search(m_lastSearch);
}

void TableModelKaraokeSongs::setSongDuration(const QString &path, unsigned int duration) {
    auto it = find_if(m_allSongs.begin(), m_allSongs.end(), [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
        return (song->path == path);
    });
    if (it == m_allSongs.end())
        return;
    it->get()->duration = static_cast<int>(duration);
    int songId = it->get()->id;
    auto it2 = find_if(m_filteredSongs.begin(), m_filteredSongs.end(),
                       [&songId](const std::shared_ptr<okj::KaraokeSong> &song) {
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
                                         [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
                                             return (song->path == path);
                                         });
    m_filteredSongs.erase(newFilteredEnd, m_filteredSongs.end());
    emit layoutChanged();

    auto songEntry = std::find_if(m_allSongs.begin(), m_allSongs.end(),
                                         [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
                                             return (song->path == path);
                                         });
    if (songEntry != m_allSongs.end())
        songEntry->get()->bad = true;
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
                                             [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
                                                 return (song->path == path);
                                             });
        m_filteredSongs.erase(newFilteredEnd, m_filteredSongs.end());

        emit layoutChanged();
        auto newAllSongsEnd = std::remove_if(m_allSongs.begin(), m_allSongs.end(),
                                             [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
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
    m_logger->debug("{} findMatchingAudioFile({}) called", m_loggingPrefix, path.toStdString());
    std::array<QString, 41> audioExtensions{
            "mp3",
            "ogg",
            "wav",
            "mov",
            "flac",
            "MP3",
            "WAV",
            "OGG",
            "MOV",
            "FLAC",
            "Mp3","mP3",
            "Wav","wAv","waV","WAv","wAV","WaV",
            "Ogg","oGg","ogG","OGg","oGG","OgG",
            "Mov","mOv","moV","MOv","mOV","MoV",
            "Flac","fLac","flAc","flaC","FLac","FLAc",
            "flAC","fLAC","FlaC", "FLaC", "FlAC"
    };
    QFileInfo cdgInfo(path);
    for (const auto &ext : audioExtensions) {
        QString testPath = cdgInfo.absolutePath() + QDir::separator() + cdgInfo.completeBaseName() + '.' + ext;
        if (QFile::exists(testPath))
            return testPath;
    }
    return {};
}

int TableModelKaraokeSongs::addSong(okj::KaraokeSong song) {
    m_logger->debug("{} addSong() called", m_loggingPrefix);
    if (int songId = getIdForPath(song.path); songId > -1) {
        m_logger->debug("{} addSong() - Song at path already exists in the db:{}", m_loggingPrefix, song.path.toStdString());
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
    if (auto error = query.lastError(); error.type() != QSqlError::NoError) {
        m_logger->error("{} Error adding song to the database", m_loggingPrefix);
        m_logger->error("{} Database error: {}", m_loggingPrefix, error.text().toStdString());
        return -1;
    } else {
        int lastInsertId = query.lastInsertId().toInt();
        song.id = lastInsertId;
        m_allSongs.push_back(std::make_shared<okj::KaraokeSong>(song));
        search(m_lastSearch);
        return lastInsertId;
    }
}
