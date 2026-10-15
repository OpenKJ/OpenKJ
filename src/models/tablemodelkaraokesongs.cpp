#include "tablemodelkaraokesongs.h"

#include <QApplication>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QUuid>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QSqlError>
#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QSvgRenderer>
#include <QMimeData>
#include <array>
#include <stdexcept>
#include <QtConcurrent/QtConcurrentRun>

std::ostream & operator<<(std::ostream& os, const QString& s);

namespace {
// Large superseded libraries and result lists must also be freed off the UI thread.
template<typename T> void releaseOffThread(std::shared_ptr<T> data) {
    if (data)
        QThreadPool::globalInstance()->start([data = std::move(data)] {});
}
}

TableModelKaraokeSongs::TableModelKaraokeSongs(QObject *parent)
        : QAbstractTableModel(parent) {
    m_logger = spdlog::get("logger");
    resizeIconsForFont(m_settings.applicationFont());
    searchTimer.setSingleShot(true);
    connect(&searchTimer, &QTimer::timeout, this, &TableModelKaraokeSongs::searchExec);
    connect(&m_searchWatcher, &QFutureWatcher<std::shared_ptr<SearchResult>>::finished, this, [this] {
        m_searchBusy = false;
        auto result = m_searchWatcher.future().takeResult();
        bool installedLibrary = false;
        if (result && result->revision == m_searchRevision->load()) {
            QElapsedTimer applyTimer;
            applyTimer.start();
            if (result->library) {
                // Changes received while the SELECT was running must survive replacement.
                auto &data = *result->library;
                for (auto it = m_durationsDuringLoad.cbegin(); it != m_durationsDuringLoad.cend(); ++it) {
                    if (auto song = data.byPath.value(it.key()).lock())
                        song->duration = it.value();
                }
                for (auto it = m_historyDuringLoad.cbegin(); it != m_historyDuringLoad.cend(); ++it) {
                    if (auto song = data.byId.value(it.key()).lock()) {
                        song->plays = it->plays;
                        song->lastPlay = it->lastPlay;
                    }
                }
                beginResetModel();
                m_allSongs.swap(data.songs);
                m_songsByPath.swap(data.byPath);
                m_songsById.swap(data.byId);
                m_searchSnapshot.swap(data.snapshot);
                m_pendingLibrary.reset();
                m_durationsDuringLoad.clear();
                m_historyDuringLoad.clear();
                m_recordsDuringLoad.clear();
                m_snapshotChanges.clear();
                installedLibrary = true;
            } else {
                if (result->snapshot) {
                    m_searchSnapshot.swap(result->snapshot);
                    m_snapshotChanges.clear();
                }
                beginResetModel();
            }
            m_filteredSongs.swap(result->songs);
            m_visibleRows.swap(result->rows);
            endResetModel();
            m_logger->info("[DatabaseTiming] Library search worker: {} ms, matches={}, UI apply: {} ms",
                           result->elapsedMs, m_filteredSongs.size(), applyTimer.elapsed());
        }
        releaseOffThread(std::move(result));
        if (installedLibrary)
            emit libraryLoadFinished(true);
        if (m_searchPending && !searchTimer.isActive())
            searchExec();
    });
    connect(&m_loadWatcher, &QFutureWatcher<std::shared_ptr<LibraryData>>::finished, this, [this] {
        m_loadBusy = false;
        auto data = m_loadWatcher.future().takeResult();
        if (m_loadPending) {
            releaseOffThread(std::move(data));
            startLibraryLoad();
            return;
        }
        if (!data || data->revision != m_loadRevision->load()) {
            releaseOffThread(std::move(data));
            return;
        }
        if (!data->error.isEmpty()) {
            const auto error = data->error;
            m_logger->error("{} Library reload failed: {}", m_loggingPrefix, error.toStdString());
            releaseOffThread(std::move(data));
            m_durationsDuringLoad.clear();
            m_historyDuringLoad.clear();
            m_recordsDuringLoad.clear();
            emit libraryLoadFailed(error);
            emit libraryLoadFinished(false);
            search(m_lastSearch);
            return;
        }
        m_logger->info("[DatabaseTiming] Library read worker: {} ms, songs={}", data->elapsedMs, data->songs.size());
        m_pendingLibrary = std::move(data);
        search(m_lastSearch);
    });
}

TableModelKaraokeSongs::~TableModelKaraokeSongs() {
    // Jobs own their inputs; cancellation never waits for the UI or the worker.
    ++*m_searchRevision;
    ++*m_loadRevision;
    if (m_loadBusy && m_loadWatcher.isFinished())
        releaseOffThread(m_loadWatcher.future().takeResult());
    if (m_searchBusy && m_searchWatcher.isFinished())
        releaseOffThread(m_searchWatcher.future().takeResult());
    releaseOffThread(std::move(m_pendingLibrary));
    auto retired = std::make_shared<SearchResult>();
    retired->library = std::make_shared<LibraryData>();
    retired->library->songs.swap(m_allSongs);
    retired->library->byPath.swap(m_songsByPath);
    retired->library->byId.swap(m_songsById);
    retired->library->snapshot.swap(m_searchSnapshot);
    retired->songs.swap(m_filteredSongs);
    retired->rows.swap(m_visibleRows);
    releaseOffThread(std::move(retired));
}

void TableModelKaraokeSongs::invalidateSearchSnapshot() {
    m_snapshotChanges.clear();
    releaseOffThread(std::move(m_searchSnapshot));
    ++*m_searchRevision;
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
    switch (role) {
        case Qt::FontRole:
            return m_itemFont;
        case Qt::TextAlignmentRole:
            return getColumnTextAlignmentHint(index.column());
        case Qt::DecorationRole:
            return getColumnDecorationRole(index);
        case Qt::DisplayRole:
            return getItemDisplayData(index);
        case Qt::SizeHintRole:
            return m_itemFontMetrics.size(Qt::TextSingleLine, getItemDisplayData(index).toString());
        case Qt::UserRole: {
            QVariant retVal;
            retVal.setValue(m_filteredSongs.at(index.row()));
            return retVal;
        }
        default:
            return {};
    }
}

QVariant TableModelKaraokeSongs::getColumnDecorationRole(const QModelIndex &index) const {
    switch (index.column()) {
        case COL_SONGID:
            if (m_filteredSongs.at(index.row())->path.endsWith("cdg", Qt::CaseInsensitive))
                return m_iconCdg;
            else if (m_filteredSongs.at(index.row())->path.endsWith("zip", Qt::CaseInsensitive))
                return m_iconZip;
            else
                return m_iconVid;
        default:
            return {};
    }
}

QVariant TableModelKaraokeSongs::getColumnTextAlignmentHint(int column) {
    switch (column) {
        case COL_DURATION:
        case COL_PLAYS:
        case COL_LASTPLAY:
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
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
    if (!m_loadBusy && !m_pendingLibrary) {
        m_durationsDuringLoad.clear();
        m_historyDuringLoad.clear();
        m_recordsDuringLoad.clear();
    }
    ++*m_loadRevision;
    ++*m_searchRevision;
    releaseOffThread(std::move(m_pendingLibrary));
    m_loadPending = true;
    m_searchPending = true;
    emit libraryLoadStarted();
    if (!m_loadBusy)
        startLibraryLoad();
}

bool TableModelKaraokeSongs::refreshSong(int songId) {
    QSqlQuery query;
    query.prepare("SELECT songid,artist,title,discid,duration,filename,path,searchstring,plays,lastplay FROM dbsongs WHERE songid=:id");
    query.bindValue(":id", songId);
    if (!query.exec() || !query.next()) {
        m_logger->warn("{} Unable to refresh song {}: {}", m_loggingPrefix, songId, query.lastError().text().toStdString());
        return false;
    }
    const okj::KaraokeSong record{
        query.value(0).toInt(),
        query.value(1).toString(), query.value(1).toString().toLower(),
        query.value(2).toString(), query.value(2).toString().toLower(),
        query.value(3).toString(), query.value(3).toString().toLower(),
        query.value(4).toInt(), query.value(5).toString(), query.value(6).toString(),
        query.value(7).toString().replace('&', " and ").toLower(),
        query.value(8).toInt(), query.value(9).toDateTime(),
        query.value(3).toString() == "!!BAD!!", query.value(3).toString() == "!!DROPPED!!"
    };
    applySongRecord(record);
    return true;
}

void TableModelKaraokeSongs::applySongRecord(const okj::KaraokeSong &record) {
    if (isLoading())
        m_recordsDuringLoad.insert(record.id, record);
    auto song = findSongById(record.id);
    if (song) {
        m_songsByPath.remove(song->path);
        const auto old = std::find(m_allSongs.begin(), m_allSongs.end(), song);
        if (old != m_allSongs.end())
            m_allSongs.erase(old);
        *song = record; // Preserve shared references held by views and menus.
    } else {
        song = std::make_shared<okj::KaraokeSong>(record);
    }
    const auto position = std::lower_bound(m_allSongs.begin(), m_allSongs.end(), song,
        [this](const auto &a, const auto &b) {
            return m_sortOrder == Qt::AscendingOrder ? songLess(a, b, m_sortColumn) : songLess(b, a, m_sortColumn);
        });
    const int beforeId = position == m_allSongs.end() ? -1 : (*position)->id;
    m_allSongs.insert(position, song);
    m_songsByPath.insert(song->path, song);
    m_songsById.insert(song->id, song);
    if (m_searchSnapshot) {
        // Copy and update the large immutable snapshot in the search worker,
        // retaining only small, ordered edits on the GUI thread.
        m_snapshotChanges.push_back({{song->searchString,
            QString(song->artistL).replace('&', " and "), QString(song->titleL).replace('&', " and "),
            song->id, song->bad || song->dropped, song}, beforeId});
    }
    const auto row = m_visibleRows.constFind(song->id);
    if (row != m_visibleRows.cend())
        emit dataChanged(index(row.value(), 0), index(row.value(), columnCount({}) - 1));
    search(m_lastSearch);
}

void TableModelKaraokeSongs::reloadIfLoading() {
    if (m_loadBusy || m_loadPending || m_pendingLibrary)
        loadData();
}

void TableModelKaraokeSongs::startLibraryLoad() {
    m_loadPending = false;
    m_loadBusy = true;
    const auto revision = m_loadRevision->load();
    const auto dbPath = QSqlDatabase::database(QLatin1String(QSqlDatabase::defaultConnection), false).databaseName();
    m_loadWatcher.setFuture(QtConcurrent::run([dbPath, revision, cancellation = m_loadRevision] {
        auto data = std::make_shared<LibraryData>();
        data->revision = revision;
        QElapsedTimer timer;
        timer.start();
        const auto connectionName = "library-" + QUuid::createUuid().toString();
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(dbPath);
            db.setConnectOptions("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=1000");
            if (!db.open()) {
                data->error = db.lastError().text();
            } else {
                QSqlQuery query(db);
                query.setForwardOnly(true);
                if (!query.exec("SELECT songid,artist,title,discid,duration,filename,path,searchstring,plays,lastplay FROM dbsongs")) {
                    data->error = query.lastError().text();
                } else {
                    while (query.next()) {
                        if ((data->songs.size() % 256) == 0 && cancellation->load() != revision)
                            break;
                        auto song = std::make_shared<okj::KaraokeSong>(okj::KaraokeSong{
                            query.value(0).toInt(),
                            query.value(1).toString(), query.value(1).toString().toLower(),
                            query.value(2).toString(), query.value(2).toString().toLower(),
                            query.value(3).toString(), query.value(3).toString().toLower(),
                            query.value(4).toInt(), query.value(5).toString(), query.value(6).toString(),
                            query.value(7).toString().replace('&', " and ").toLower(),
                            query.value(8).toInt(), query.value(9).toDateTime(),
                            query.value(3).toString() == "!!BAD!!", query.value(3).toString() == "!!DROPPED!!"
                        });
                        data->byPath.insert(song->path, song);
                        data->byId.insert(song->id, song);
                        data->songs.push_back(std::move(song));
                    }
                    if (query.lastError().isValid())
                        data->error = query.lastError().text();
                }
            }
        }
        QSqlDatabase::removeDatabase(connectionName);
        data->elapsedMs = timer.elapsed();
        return data;
    }));
}

void TableModelKaraokeSongs::search(const QString &searchString) {
    ++*m_searchRevision;
    m_searchPending = true;
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
    if (m_searchBusy || m_loadBusy || m_loadPending)
        return; // One running job and one latest pending request, never a job per key.
    if (m_pendingLibrary && !m_recordsDuringLoad.isEmpty()) {
        // No search job is active here, so the replacement belongs to the GUI
        // thread until it is handed back to the worker for sorting/filtering.
        auto &library = *m_pendingLibrary;
        for (auto it = m_recordsDuringLoad.cbegin(); it != m_recordsDuringLoad.cend(); ++it) {
            auto song = library.byId.value(it.key()).lock();
            if (song) {
                library.byPath.remove(song->path);
                *song = it.value();
            } else {
                song = std::make_shared<okj::KaraokeSong>(it.value());
                library.songs.push_back(song);
                library.byId.insert(song->id, song);
            }
            library.byPath.insert(song->path, song);
        }
        releaseOffThread(std::move(library.snapshot));
    }
    if (!m_searchSnapshot && !m_pendingLibrary) {
        auto snapshot = std::make_shared<std::vector<SearchEntry>>();
        snapshot->reserve(m_allSongs.size());
        for (const auto &song : m_allSongs) {
            snapshot->push_back({song->searchString,
                                 QString(song->artistL).replace('&', " and "),
                                 QString(song->titleL).replace('&', " and "),
                                 song->id, song->bad || song->dropped, song});
        }
        m_searchSnapshot = std::move(snapshot);
    }
    m_searchPending = false;
    m_searchBusy = true;
    const auto revision = m_searchRevision->load();
    const auto needles = m_lastSearch.split(' ', Qt::SkipEmptyParts);
    const auto type = m_searchType;
    const bool ignoreApos = m_settings.ignoreAposInSearch();
    const auto sortColumn = m_sortColumn;
    const auto sortOrder = m_sortOrder;
    m_searchWatcher.setFuture(QtConcurrent::run(
        [snapshot = m_searchSnapshot, library = m_pendingLibrary, cancellation = m_searchRevision,
         revision, needles, type, ignoreApos, sortColumn, sortOrder, changes = m_snapshotChanges]() mutable {
            QElapsedTimer timer;
            timer.start();
            if (library) {
                if (!library->snapshot || library->sortColumn != sortColumn || library->sortOrder != sortOrder) {
                    sortSongs(library->songs, sortColumn, sortOrder);
                    auto entries = std::make_shared<std::vector<SearchEntry>>();
                    entries->reserve(library->songs.size());
                    for (const auto &song : library->songs) {
                        if ((entries->size() % 256) == 0 && cancellation->load() != revision)
                            return std::shared_ptr<SearchResult>{};
                        entries->push_back({song->searchString,
                                            QString(song->artistL).replace('&', " and "),
                                            QString(song->titleL).replace('&', " and "),
                                            song->id, song->bad || song->dropped, song});
                    }
                    library->snapshot = std::move(entries);
                    library->sortColumn = sortColumn;
                    library->sortOrder = sortOrder;
                }
                snapshot = library->snapshot;
            }
            if (!library && !changes.empty()) {
                auto updated = std::make_shared<std::vector<SearchEntry>>(*snapshot);
                for (const auto &change : changes) {
                    if (cancellation->load() != revision)
                        return std::shared_ptr<SearchResult>{};
                    updated->erase(std::remove_if(updated->begin(), updated->end(),
                        [&change](const auto &entry) { return entry.id == change.entry.id; }), updated->end());
                    const auto position = std::find_if(updated->begin(), updated->end(),
                        [&change](const auto &entry) { return entry.id == change.beforeId; });
                    updated->insert(position, change.entry);
                }
                snapshot = std::move(updated);
            }
            auto result = std::make_shared<SearchResult>();
            result->library = library;
            if (!library && !changes.empty())
                result->snapshot = snapshot;
            result->revision = revision;
            result->songs.reserve(snapshot->size());
            result->rows.reserve(static_cast<int>(snapshot->size()));
            size_t checked = 0;
            for (const auto &entry : *snapshot) {
                if ((checked++ % 256) == 0 && cancellation->load() != revision)
                    return std::shared_ptr<SearchResult>{};
                if (entry.excluded)
                    continue;
                QString haystack;
                switch (type) {
                    case SEARCH_TYPE_ALL: haystack = entry.all; break;
                    case SEARCH_TYPE_ARTIST: haystack = entry.artist; break;
                    case SEARCH_TYPE_TITLE: haystack = entry.title; break;
                }
                if (ignoreApos)
                    haystack.remove('\'');
                bool match = true;
                for (const auto &needle : needles) {
                    if (!haystack.contains(needle)) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    result->rows.insert(entry.id, static_cast<int>(result->songs.size()));
                    result->songs.push_back(entry.song);
                }
            }
            result->elapsedMs = timer.elapsed();
            return result;
        }));
}

void TableModelKaraokeSongs::setSearchType(TableModelKaraokeSongs::SearchType type) {
    if (m_searchType == type)
        return;
    m_searchType = type;
    search(m_lastSearch);
}

int TableModelKaraokeSongs::getIdForPath(const QString &path) {
    const auto entry = m_songsByPath.constFind(path);
    if (entry == m_songsByPath.cend())
        return -1;
    const auto song = entry.value().lock();
    return song ? song->id : -1;
}

std::shared_ptr<okj::KaraokeSong> TableModelKaraokeSongs::findSongById(int songId) const {
    const auto entry = m_songsById.constFind(songId);
    return entry == m_songsById.cend() ? nullptr : entry.value().lock();
}

QString TableModelKaraokeSongs::getPath(const int songId) {
    const auto song = findSongById(songId);
    return song ? song->path : QString{};
}

void TableModelKaraokeSongs::updateSongHistory(const int songId) {
    const auto now = QDateTime::currentDateTime();
    const auto song = findSongById(songId);
    if (song) {
        ++song->plays;
        song->lastPlay = now;
        if (m_loadBusy || m_loadPending || m_pendingLibrary)
            m_historyDuringLoad.insert(songId, {song->plays, now});
        const auto row = m_visibleRows.constFind(songId);
        if (row != m_visibleRows.cend()) {
            emit dataChanged(index(row.value(), COL_PLAYS), index(row.value(), COL_LASTPLAY),
                             QVector<int>(Qt::DisplayRole));
        }
    }

    QSqlQuery query;
    query.prepare("UPDATE dbSongs set plays = plays + :incVal, lastplay = :curTs WHERE songid = :songid");
    query.bindValue(":curTs", now);
    query.bindValue(":songid", songId);
    query.bindValue(":incVal", 1);
    query.exec();
    if (!song && isLoading()) {
        // Playback from an existing queue can begin before the initial library
        // is installed. Read back its committed count for the pending snapshot.
        query.prepare("SELECT plays, lastplay FROM dbsongs WHERE songid = :songid");
        query.bindValue(":songid", songId);
        if (query.exec() && query.next())
            m_historyDuringLoad.insert(songId, {query.value(0).toInt(), query.value(1).toDateTime()});
    }
}

okj::KaraokeSong &TableModelKaraokeSongs::getSong(const int songId) {
    const auto song = findSongById(songId);
    if (!song)
        throw std::out_of_range("Song ID is absent from the karaoke library");
    return *song;
}

void TableModelKaraokeSongs::resizeIconsForFont(const QFont &font) {
    m_logger->trace("{} resizeIconsForFont called with font: {} size: {}", m_loggingPrefix, font.toString().toStdString(), QFontMetrics(font).height());
    m_itemFont = m_settings.applicationFont();
    m_headerFont = m_settings.applicationFont();
    m_headerFont.setBold(true);
    m_itemFontMetrics = QFontMetrics(m_itemFont);
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
    QSvgRenderer svgRndrVid(thm + "mimetypes/22/video-mp4.svg");
    QSvgRenderer svgRndrZip(thm + "mimetypes/22/application-zip.svg");
    QSvgRenderer svgRndrCdg(thm + "mimetypes/22/application-x-cda.svg");
    svgRndrVid.render(&painterVid);
    svgRndrZip.render(&painterZip);
    svgRndrCdg.render(&painterCdg);
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


bool TableModelKaraokeSongs::songLess(const std::shared_ptr<okj::KaraokeSong> &a,
                                     const std::shared_ptr<okj::KaraokeSong> &b, int column) {
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
}

void TableModelKaraokeSongs::sortSongs(std::vector<std::shared_ptr<okj::KaraokeSong>> &songs,
                                     int column, Qt::SortOrder order) {
    auto sortLambda = [column](const auto &a, const auto &b) { return songLess(a, b, column); };

    if (order == Qt::AscendingOrder) {
        std::sort(songs.begin(), songs.end(), sortLambda);
    } else {
        std::sort(songs.rbegin(), songs.rend(), sortLambda);
    }
}

void TableModelKaraokeSongs::sort(int column, Qt::SortOrder order) {
    m_sortColumn = column;
    m_sortOrder = order;
    invalidateSearchSnapshot();
    QElapsedTimer timer;
    timer.start();
    // A replacement will be sorted by the worker with the latest header choice.
    if (!m_loadBusy && !m_loadPending && !m_pendingLibrary) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
        sortSongs(m_allSongs, column, order);
        QApplication::restoreOverrideCursor();
    }
    m_logger->info("[DatabaseTiming] Library sort: {} ms, songs={}, column={}",
                   timer.elapsed(), m_allSongs.size(), column);
    search(m_lastSearch);
}

void TableModelKaraokeSongs::rebuildVisibleRows() {
    m_visibleRows.clear();
    m_visibleRows.reserve(static_cast<int>(m_filteredSongs.size()));
    for (size_t row = 0; row < m_filteredSongs.size(); ++row)
        m_visibleRows.insert(m_filteredSongs[row]->id, static_cast<int>(row));
}

void TableModelKaraokeSongs::setSongDuration(const QString &path, unsigned int duration) {
    if (duration > 0 && (m_loadBusy || m_loadPending || m_pendingLibrary))
        m_durationsDuringLoad.insert(path, static_cast<int>(duration));
    const auto entry = m_songsByPath.constFind(path);
    if (entry == m_songsByPath.cend())
        return;
    auto song = entry.value().lock();
    if (!song || song->duration > 0)
        return;
    song->duration = static_cast<int>(duration);
    const auto row = m_visibleRows.constFind(song->id);
    if (row != m_visibleRows.cend())
        emit dataChanged(index(row.value(), COL_DURATION), index(row.value(), COL_DURATION),
                         QVector<int>(Qt::DisplayRole));
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
    rebuildVisibleRows();
    emit layoutChanged();

    auto songEntry = std::find_if(m_allSongs.begin(), m_allSongs.end(),
                                         [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
                                             return (song->path == path);
                                         });
    if (songEntry != m_allSongs.end())
        songEntry->get()->bad = true;
    invalidateSearchSnapshot();
    search(m_lastSearch);
    reloadIfLoading();
}

TableModelKaraokeSongs::DeleteStatus TableModelKaraokeSongs::removeBadSong(QString path) {
    const int removedSongId = getIdForPath(path);
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

        rebuildVisibleRows();
        emit layoutChanged();
        auto newAllSongsEnd = std::remove_if(m_allSongs.begin(), m_allSongs.end(),
                                             [&path](const std::shared_ptr<okj::KaraokeSong> &song) {
                                                 return (song->path == path);
                                             });
        m_songsById.remove(removedSongId);
        m_allSongs.erase(newAllSongsEnd, m_allSongs.end());
        m_songsByPath.remove(path);
        invalidateSearchSnapshot();
        search(m_lastSearch);
        reloadIfLoading();

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
        refreshSong(lastInsertId);
        return lastInsertId;
    }
}
