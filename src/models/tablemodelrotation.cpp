#include "tablemodelrotation.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSvgRenderer>
#include <QMimeData>
#include <QJsonArray>
#include <QJsonDocument>
#include <spdlog/spdlog.h>


TableModelRotation::TableModelRotation(QObject *parent)
        : QAbstractTableModel(parent) {
    m_logger = spdlog::get("logger");
    resizeIconsForFont(m_settings.applicationFont());
    m_rotationTopSingerId = m_settings.lastRunRotationTopSingerId();
}

QVariant TableModelRotation::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::SizeHintRole) {
        switch (section) {
            case COL_REGULAR:
            case COL_DELETE:
            case COL_ID:
                return QSize(QFontMetrics(m_settings.applicationFont()).height() * 2, QFontMetrics(m_settings.applicationFont()).height());
            default:
                return {};
        }
    }
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case COL_ID:
                return {};
            case COL_NAME:
                return "Name";
            case COL_NEXT_SONG:
                return "Next Song";
            default:
                return {};

        }
    }
    return {};
}

int TableModelRotation::rowCount([[maybe_unused]]const QModelIndex &parent) const {
    return static_cast<int>(m_singers.size());
}

int TableModelRotation::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    return 7;
}

QVariant TableModelRotation::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return {};
    if (role == Qt::ToolTipRole) {
        QString toolTipText;
        auto curSingerPos{0};
        if (m_currentSingerId != -1)
            curSingerPos = getSinger(m_currentSingerId).position;
        auto hoverSingerPos{index.sibling(index.row(), COL_POSITION).data().toInt()};
        int totalWaitDuration = 0;
        int singerId = index.data(Qt::UserRole).toInt();
        int qSongsSung = numSongsSung(singerId);
        int qSongsUnsung = numSongsUnsung(singerId);
        if (m_currentSingerId == singerId) {
            toolTipText = "Current singer - Sung: " + QString::number(qSongsSung) + " - Unsung: " +
                          QString::number(qSongsUnsung);
        } else if (curSingerPos < hoverSingerPos) {
            toolTipText = "Wait: " + QString::number(hoverSingerPos - curSingerPos) + " - Sung: " +
                          QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
            for (int i = curSingerPos; i < hoverSingerPos; i++) {
                int sId = getSingerAtPosition(i).id;
                if (i == curSingerPos) {
                    totalWaitDuration += m_remainSecs;
                } else if (sId != singerId) {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
        } else if (curSingerPos > hoverSingerPos) {
            toolTipText = "Wait: " + QString::number(hoverSingerPos + (m_singers.size() - curSingerPos)) + " - Sung: " +
                          QString::number(qSongsSung) + " - Unsung: " + QString::number(qSongsUnsung);
            for (int i = 0; i < hoverSingerPos; i++) {
                int sId = getSingerAtPosition(i).id;
                if (sId != singerId) {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
            for (int i = curSingerPos; i < m_singers.size(); i++) {
                int sId = getSingerAtPosition(i).id;
                if (i == curSingerPos)
                    totalWaitDuration += 240;
                else if (sId != singerId) {
                    int nextDuration = nextSongDurationSecs(sId);
                    totalWaitDuration += nextDuration;
                }
            }
        }
        toolTipText += "\nTime Added: " + m_singers.at(index.row()).addTs.toString("h:mm a");

        if (totalWaitDuration > 0) {
            int minutes = totalWaitDuration / 60;
            int seconds = totalWaitDuration % 60;
            if (seconds > 0)
                minutes++;
            if (minutes > 60) {
                int hours = minutes / 60;
                minutes = minutes % 60;
                if (hours > 1)
                    toolTipText += "\nEst wait time: " + QString::number(hours) + " hours " + QString::number(minutes) +
                                   " min";
                else
                    toolTipText +=
                            "\nEst wait time: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
            } else
                toolTipText += "\nEst wait time: " + QString::number(minutes) + " min";
        }
        return QString(toolTipText);
    }

    if (role == Qt::UserRole) {
        return m_singers.at(index.row()).id;
    }
    if (role == Qt::SizeHintRole) {
        switch (index.column()) {
            case COL_REGULAR:
            case COL_DELETE:
            case COL_ID:
                auto fHeight = QFontMetrics(m_settings.applicationFont()).height();
                return QSize(fHeight, fHeight);
        }
    }
    if (role == Qt::DecorationRole && index.column() == COL_NAME) {
        if (numSongsUnsung(index.data(Qt::UserRole).toInt()) > 0)
            return m_iconGreenCircle;
        return m_iconYellowCircle;
    }
    if (role == Qt::TextAlignmentRole && index.column() == COL_ID)
        return Qt::AlignCenter;
    if (role == Qt::BackgroundRole && m_singers.at(index.row()).id == m_currentSingerId) {
        if (index.column() > 0)
            return (m_settings.theme() == 1) ? QColor(180, 180, 0) : QColor("yellow");
    }

    if (role == Qt::BackgroundRole && index.column() == COL_NAME) {
        int singerId = index.data(Qt::UserRole).toInt();
        int qSongsSung = numSongsSung(singerId);
        if (singerId == m_rotationTopSingerId && m_settings.rotationAltSortOrder())
            return QColor("green");
        if (qSongsSung == 0)
            return QColor(140, 30, 150);
    }
    if (role == Qt::ForegroundRole && m_singers.at(index.row()).id == m_currentSingerId) {
        if (index.column() > 0)
            return QColor("black");
    }
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case COL_ID:
                if (m_settings.rotationDisplayPosition()) {
                    int curSingerPos{getSinger(m_currentSingerId).position};
                    size_t wait{0};
                    if (curSingerPos < index.row())
                        wait = index.row() - curSingerPos;
                    else if (curSingerPos > index.row())
                        wait = index.row() + (m_singers.size() - curSingerPos);
                    if (wait > 0)
                        return static_cast<int>(wait);
                    return {};
                }
                return {};
            case COL_NAME:
                return m_singers.at(index.row()).name;
            case COL_POSITION:
                return m_singers.at(index.row()).position;
            case COL_REGULAR:
                return m_singers.at(index.row()).regular;
            case COL_ADDTS:
                return m_singers.at(index.row()).addTs;
            case COL_NEXT_SONG:
                if (m_settings.rotationShowNextSong())
                    return nextSongArtistTitle(index.data(Qt::UserRole).toInt());
                return {};
        }
    }
    return {};
}

void TableModelRotation::loadData() {
    m_logger->debug("{} loading rotation data from disk", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    m_singers.clear();
    QSqlQuery query;
    query.exec("SELECT singerid,name,position,regular,addts FROM rotationsingers ORDER BY position");
    qInfo() << "TableModelRotation - SQL error on load: " << query.lastError();
    while (query.next()) {
        m_singers.emplace_back(RotationSinger{
                query.value(0).toInt(),
                query.value(1).toString(),
                query.value(2).toInt(),
                query.value(3).toBool(),
                query.value(4).toDateTime()
        });
    }
    emit layoutChanged();
    m_logger->debug("{} loaded {} rotation singers", m_loggingPrefix, m_singers.size());
}

void TableModelRotation::commitChanges() {
    m_logger->debug("{} Committing db changes to disk", m_loggingPrefix);
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM rotationsingers");
    query.prepare(
            "INSERT INTO rotationsingers (singerid,name,position,regular,regularid,addts) VALUES(:singerid,:name,:pos,:regular,:regularid,:addts)");
    std::for_each(m_singers.begin(), m_singers.end(), [&](RotationSinger &singer) {
        query.bindValue(":singerid", singer.id);
        query.bindValue(":name", singer.name);
        query.bindValue(":pos", singer.position);
        query.bindValue(":regular", singer.regular);
        query.bindValue(":regularid", -1);
        query.bindValue(":addts", singer.addTs);
        query.exec();
    });
    query.exec("COMMIT");
    if ( auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} Commit error! Unable to write rotation changes to db on disk! Error: {}", m_loggingPrefix, lastError.text().toStdString());
    else
        m_logger->debug("{} Commit completed successfully", m_loggingPrefix);
}

int TableModelRotation::singerAdd(const QString &name, const int positionHint) {
    m_logger->debug("{} Adding singer {} to rotation using positionHint {}", m_loggingPrefix, name.toStdString(), positionHint);
    auto curTs = QDateTime::currentDateTime();
    int addPos = static_cast<int>(m_singers.size());
    QSqlQuery query;
    query.prepare(
            "INSERT INTO rotationsingers (name,position,regular,regularid,addts) VALUES(:name,:pos,:regular,:regularid,:addts)");
    query.bindValue(":name", name);
    query.bindValue(":pos", addPos);
    query.bindValue(":regular", false);
    query.bindValue(":regularid", -1);
    query.bindValue(":addts", curTs);
    query.exec();
    if ( auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} Commit error! Unable to write rotation changes to db on disk while adding singer! Error: {}", m_loggingPrefix, lastError.text().toStdString());
    int singerId = query.lastInsertId().toInt();
    if (m_singers.empty()) {
        m_rotationTopSingerId = singerId;
        m_settings.setLastRunRotationTopSingerId(singerId);
    }
    if (singerId == -1) {
        m_logger->critical("{} Error occurred while inserting singer into the database singers table!!!", m_loggingPrefix);
        return -1;
    }
    emit layoutAboutToBeChanged();
    m_singers.emplace_back(RotationSinger{
            singerId,
            name,
            addPos,
            false,
            curTs
    });
    emit layoutChanged();

    int curSingerPos = getSinger(m_currentSingerId).position;
    switch (positionHint) {
        case ADD_FAIR: {
            if (curSingerPos > 0 && !m_settings.rotationAltSortOrder())
                singerMove(addPos, curSingerPos);
            break;
        }
        case ADD_NEXT:
            if (curSingerPos != m_singers.size() - 2)
                singerMove(addPos, curSingerPos + 1);
            break;
        case ADD_BOTTOM:
            if (m_settings.rotationAltSortOrder()) {
                if (auto rotTopSingerPos = getSinger(m_rotationTopSingerId).position; rotTopSingerPos > 0)
                    singerMove(addPos, rotTopSingerPos);
            }
        default:
            break;
    }


    emit rotationModified();
    m_logger->debug("{} Singer add completed", m_loggingPrefix);
    outputRotationDebug();
    return singerId;
}

void TableModelRotation::singerMove(const int oldPosition, const int newPosition, const bool skipCommit) {
    if (oldPosition == newPosition)
        return;
    if (auto singer = getSingerAtPosition(oldPosition); singer.isValid())
        m_logger->debug("{} Moving singer - Name: {} - Old position: {} - New position: {} - Skip DB commit: {}",
                        m_loggingPrefix, singer.name.toStdString(), oldPosition, newPosition, skipCommit);
    else
        m_logger->error("{} Error loading singer by position!!");
    emit layoutAboutToBeChanged();
    if (oldPosition > newPosition) {
        // moving up
        std::for_each(m_singers.begin(), m_singers.end(), [&oldPosition, &newPosition](RotationSinger &singer) {
            if (singer.position == oldPosition)
                singer.position = newPosition;
            else if (singer.position >= newPosition && singer.position < oldPosition)
                singer.position++;
        });
    } else {
        // moving down
        std::for_each(m_singers.begin(), m_singers.end(), [&oldPosition, &newPosition](RotationSinger &singer) {
            if (singer.position == oldPosition)
                singer.position = newPosition;
            else if (singer.position > oldPosition && singer.position <= newPosition)
                singer.position--;
        });
    }
    std::sort(m_singers.begin(), m_singers.end(), [](RotationSinger &a, RotationSinger &b) {
        return (a.position < b.position);
    });
    if (!skipCommit)
        commitChanges();
    emit layoutChanged();
    emit rotationModified();
    m_logger->debug("{} Singer move completed.", m_loggingPrefix);
    outputRotationDebug();
}

void TableModelRotation::singerSetName(const int singerId, const QString &newName) {
    m_logger->debug("{} Renaming singer '{}' to '{}'", m_loggingPrefix, getSinger(singerId).name.toStdString(),
                    newName.toStdString());
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId](RotationSinger &singer) {
        return (singer.id == singerId);
    });
    if (it == m_singers.end()) {
        m_logger->critical("{} Unable to find singer!!!", m_loggingPrefix);
        return;
    }
    it->name = newName;
    emit dataChanged(this->index(it->position, COL_NAME), this->index(it->position, COL_NAME),
                     QVector<int>{Qt::DisplayRole});
    QSqlQuery query;
    query.prepare("UPDATE rotationsingers SET name = :name WHERE singerid = :singerid");
    query.bindValue(":name", newName);
    query.bindValue(":singerid", singerId);
    query.exec();
    emit rotationModified();
    outputRotationDebug();
}

void TableModelRotation::singerDelete(const int singerId) {
    m_logger->debug("{} Deleting singer id: {} name: {}", m_loggingPrefix, singerId, getSinger(singerId).name.toStdString());
    if (singerId == m_rotationTopSingerId) {
        if (m_singers.size() == 1)
            m_rotationTopSingerId = -1;
        else if (getSinger(singerId).position == m_singers.size() - 1)
            m_rotationTopSingerId = getSingerAtPosition(0).id;
        else
            m_rotationTopSingerId = getSingerAtPosition(getSinger(singerId).position + 1).id;
        m_settings.setLastRunRotationTopSingerId(m_rotationTopSingerId);
    }

    emit layoutAboutToBeChanged();
    auto it = std::remove_if(m_singers.begin(), m_singers.end(), [&singerId](RotationSinger &singer) {
        return (singer.id == singerId);
    });
    m_singers.erase(it, m_singers.end());
    int pos{0};
    std::for_each(m_singers.begin(), m_singers.end(), [&pos](RotationSinger &singer) {
        singer.position = pos++;
    });
    emit layoutChanged();
    emit rotationModified();
    commitChanges();
    outputRotationDebug();
}

bool TableModelRotation::singerExists(const QString &name) {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name](RotationSinger &singer) {
        return (name.toLower() == singer.name.toLower());
    });
    return (it != m_singers.end());
}

void TableModelRotation::singerSetRegular(const int singerId, const bool isRegular) {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId](RotationSinger &singer) {
        return (singerId == singer.id);
    });
    it->regular = isRegular;
    emit dataChanged(this->index(it->position, COL_REGULAR), this->index(it->position, COL_REGULAR),
                     QVector<int>{Qt::DisplayRole});
    QSqlQuery query;
    query.prepare("UPDATE rotationsingers SET regular = :regular WHERE singerid = :singerid");
    query.bindValue(":regular", isRegular);
    query.bindValue(":singerid", singerId);
    query.exec();
}

void TableModelRotation::singerMakeRegular(const int singerId) {
    singerSetRegular(singerId, true);
}

void TableModelRotation::singerDisableRegularTracking(const int singerId) {
    singerSetRegular(singerId, false);
}

bool TableModelRotation::historySingerExists(const QString &name) {
    auto hSingers = historySingers();
    auto it = std::find_if(hSingers.begin(), hSingers.end(), [&name](QString &hSinger) {
        return (name.toLower() == hSinger.toLower());
    });
    return (it != hSingers.end());
}

QStringList TableModelRotation::singers() {
    QStringList names;
    names.reserve(static_cast<int>(m_singers.size()));
    std::for_each(m_singers.begin(), m_singers.end(), [&names](RotationSinger &singer) {
        names.push_back(singer.name);
    });
    return names;
}

QStringList TableModelRotation::historySingers() {
    QStringList names;
    QSqlQuery query;
    query.exec("SELECT name FROM historySingers");
    while (query.next())
        names << query.value(0).toString();
    return names;
}

QString TableModelRotation::nextSongPath(const int singerId) {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.path FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return {};
}

QString TableModelRotation::nextSongArtist(const int singerId) {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.artist FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return {};
}

QString TableModelRotation::nextSongTitle(const int singerId) {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return {};
}

QString TableModelRotation::nextSongArtistTitle(const int singerId) {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.artist, dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString() + " - " + query.value(1).toString();
    return " - empty - ";
}

QString TableModelRotation::nextSongSongId(const int singerId) {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.discid FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toString();
    return {};
}

int TableModelRotation::nextSongDurationSecs(const int singerId) const {
    QSqlQuery query;
    query.prepare(
            "SELECT dbsongs.duration FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return (query.value(0).toInt() / 1000) + m_settings.estimationSingerPad();
    else if (!m_settings.estimationSkipEmptySingers())
        return m_settings.estimationEmptySongLength() + m_settings.estimationSingerPad();
    return 0;
}

int TableModelRotation::rotationDuration() {
    int secs = 0;
    std::for_each(m_singers.begin(), m_singers.end(), [&](RotationSinger &singer) {
        secs += nextSongDurationSecs(singer.id);
    });
    return secs;
}

int TableModelRotation::nextSongKeyChg(const int singerId) {
    QSqlQuery query;
    query.prepare("SELECT keychg FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return 0;
}

int TableModelRotation::nextSongQueueId(const int singerId) {
    QSqlQuery query;
    query.prepare("SELECT qsongid FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

void TableModelRotation::clearRotation() {
    m_logger->debug("{} Clearing rotation", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    QSqlQuery query;
    query.exec("DELETE from queuesongs");
    query.exec("DELETE FROM rotationsingers");
    m_singers.clear();
    m_settings.setCurrentRotationPosition(-1);
    m_currentSingerId = -1;
    emit layoutChanged();
    emit rotationModified();
}

int TableModelRotation::currentSinger() const {
    return m_currentSingerId;
}

void TableModelRotation::setCurrentSinger(const int currentSingerId) {
    m_logger->debug("{} Setting singer id: {} name: '{}' as the current rotation singer", m_loggingPrefix, currentSingerId,
                    getSinger(currentSingerId).name.toStdString());
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
    m_settings.setCurrentRotationPosition(currentSingerId);
}

int TableModelRotation::numSongsSung(const int singerId) {
    QSqlQuery query;
    query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = true");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

int TableModelRotation::numSongsUnsung(const int singerId) {
    QSqlQuery query;
    query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = false");
    query.bindValue(":singerid", singerId);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    return -1;
}

void TableModelRotation::outputRotationDebug() {
    m_logger->debug("{} -- Rotation debug output --", m_loggingPrefix);
    m_logger->debug("{} singerid    position    regular    added                name", m_loggingPrefix);
    int expectedPosition = 0;
    bool needsRepair = false;
    std::for_each(m_singers.begin(), m_singers.end(), [&](RotationSinger &singer) {
        m_logger->debug("{}      {:03}         {:03}     {:5}     {}    {}", m_loggingPrefix, singer.id, singer.position, singer.regular, singer.addTs.toString("dd.MM.yy hh:mm:ss").toStdString(), singer.name.toStdString());
        if (singer.position != expectedPosition) {
            needsRepair = true;
            m_logger->critical("{} ERROR DETECTED!!! - Singer position does not match expected position!", m_loggingPrefix);
        }
        expectedPosition++;
    });
    m_logger->debug("{} -- Rotation debug output end --", m_loggingPrefix);
    if (needsRepair)
        fixSingerPositions();}

void TableModelRotation::fixSingerPositions() {
    m_logger->error("{} Attempting to recover from corrupted rotation data", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    int pos{0};
    std::for_each(m_singers.begin(), m_singers.end(), [&pos](RotationSinger &singer) {
        singer.position = pos++;
    });
    emit layoutChanged();
    commitChanges();
    m_logger->error("{} Repair complete", m_loggingPrefix);
}

void TableModelRotation::resizeIconsForFont(const QFont &font) {
    m_curFontHeight = QFontMetrics(font).height();
    m_iconGreenCircle = QImage(m_curFontHeight / 3, m_curFontHeight / 3, QImage::Format_ARGB32);
    m_iconYellowCircle = QImage(m_curFontHeight / 3, m_curFontHeight / 3, QImage::Format_ARGB32);
    m_iconGreenCircle.fill(Qt::transparent);
    m_iconYellowCircle.fill(Qt::transparent);
    QPainter painterGreenCircle(&m_iconGreenCircle);
    QPainter painterYellowCircle(&m_iconYellowCircle);
    painterGreenCircle.setBrush(Qt::green);
    painterGreenCircle.drawEllipse(m_iconGreenCircle.rect().center(), m_curFontHeight / 6, m_curFontHeight / 6);
    painterYellowCircle.setBrush(Qt::darkGray);
    painterYellowCircle.drawEllipse(m_iconYellowCircle.rect().center(), m_curFontHeight / 6, m_curFontHeight / 6);
}

void ItemDelegateRotation::resizeIconsForFont(const QFont &font) {
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    m_curFontHeight = QFontMetrics(font).height();
    m_iconDelete = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconCurSinger = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconRegularOn = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconRegularOff = QImage(m_curFontHeight, m_curFontHeight, QImage::Format_ARGB32);
    m_iconDelete.fill(Qt::transparent);
    m_iconCurSinger.fill(Qt::transparent);
    m_iconRegularOn.fill(Qt::transparent);
    m_iconRegularOff.fill(Qt::transparent);
    QPainter painterDelete(&m_iconDelete);
    QPainter painterPlaying(&m_iconCurSinger);
    QPainter painterRegularOn(&m_iconRegularOn);
    QPainter painterRegularOff(&m_iconRegularOff);
    QSvgRenderer svgRendererDelete(thm + "actions/16/edit-delete.svg");
    QSvgRenderer svgRendererCurSinger(thm + "status/16/mic-on.svg");
    QSvgRenderer svgRendererRegularOn(thm + "actions/16/im-user-online.svg");
    QSvgRenderer svgRendererRegularOff(thm + "actions/16/im-user.svg");
    svgRendererDelete.render(&painterDelete);
    svgRendererCurSinger.render(&painterPlaying);
    svgRendererRegularOn.render(&painterRegularOn);
    svgRendererRegularOff.render(&painterRegularOff);
}

[[maybe_unused]] ItemDelegateRotation::ItemDelegateRotation(QObject *parent) :
        QItemDelegate(parent) {
    resizeIconsForFont(m_settings.applicationFont());
}

void
ItemDelegateRotation::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (index.column() == TableModelRotation::COL_ID) {
        if (index.data(Qt::UserRole).toInt() == m_currentSinger) {
            int topPad = (option.rect.height() - m_curFontHeight) / 2;
            int leftPad = (option.rect.width() - m_curFontHeight) / 2;
            painter->drawImage(
                    QRect(option.rect.x() + leftPad, option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),
                    m_iconCurSinger);
            return;
        }
    } else if (index.column() == TableModelRotation::COL_REGULAR) {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        if (index.data().toBool())
            painter->drawImage(
                    QRect(option.rect.x() + leftPad, option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),
                    m_iconRegularOn);
        else
            painter->drawImage(
                    QRect(option.rect.x() + leftPad, option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),
                    m_iconRegularOff);
        return;
    } else if (index.column() == TableModelRotation::COL_DELETE) {
        int topPad = (option.rect.height() - m_curFontHeight) / 2;
        int leftPad = (option.rect.width() - m_curFontHeight) / 2;
        painter->drawImage(QRect(option.rect.x() + leftPad, option.rect.y() + topPad, m_curFontHeight, m_curFontHeight),
                           m_iconDelete);
        return;
    }
    QItemDelegate::paint(painter, option, index);
}

void ItemDelegateRotation::setCurrentSinger(const int singerId) {
    m_currentSinger = singerId;
}


QStringList TableModelRotation::mimeTypes() const {
    return {
            "integer/songid",
            "integer/rotationpos",
            "application/rotsingers"
    };
}

QMimeData *TableModelRotation::mimeData(const QModelIndexList &indexes) const {
    auto mimeData = new QMimeData();
    mimeData->setData("integer/rotationpos",
                      indexes.at(0).sibling(indexes.at(0).row(), COL_POSITION).data().toByteArray().data());
    if (!indexes.empty()) {
        QJsonArray jArr;
        std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
            // only act on one column to avoid duplicate singerIds in the list for each col
            if (index.column() != COL_NAME)
                return;
            jArr.append(index.data(Qt::UserRole).toInt());
        });
        QJsonDocument jDoc(jArr);
        mimeData->setData("application/rotsingers", jDoc.toJson());
    }
    return mimeData;
}

bool TableModelRotation::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                         const QModelIndex &parent) const {
    if (parent.row() == -1 && !data->hasFormat("integer/rotationpos"))
        return false;
    if ((data->hasFormat("integer/songid")) || (data->hasFormat("integer/rotationpos")))
        return true;
    return false;
}

bool TableModelRotation::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                      const QModelIndex &parent) {
    if (action == Qt::MoveAction && data->hasFormat("application/rotsingers")) {
        QJsonDocument jDoc = QJsonDocument::fromJson(data->data("application/rotsingers"));
        QJsonArray jArr = jDoc.array();
        auto ids = jArr.toVariantList();
        size_t dropRow;
        if (parent.row() >= 0)
            dropRow = parent.row();
        else if (row >= 0)
            dropRow = row;
        else
            dropRow = m_singers.size() - 1;
        if (getSinger(ids.at(0).toInt()).position > dropRow)
            std::reverse(ids.begin(), ids.end());
        std::for_each(ids.begin(), ids.end(), [&](auto val) {
            singerMove(getSinger(val.toInt()).position, dropRow, false);
        });
        commitChanges();
        emit rotationModified();
        if (dropRow == m_singers.size() - 1) {
            // moving to bottom
            emit singersMoved(static_cast<int>(m_singers.size() - ids.size()), 0, static_cast<int>(m_singers.size() - 1), columnCount(QModelIndex()) - 1);
        } else if (getSinger(ids.at(0).toInt()).position < dropRow) {
            // moving down
            emit singersMoved(static_cast<int>(dropRow - ids.size() + 1), 0, static_cast<int>(dropRow), columnCount(QModelIndex()) - 1);
        } else {
            // moving up
            emit singersMoved(static_cast<int>(dropRow), 0, static_cast<int>(dropRow + ids.size() - 1), columnCount(QModelIndex()) - 1);
        }
        return true;
    }

    if (data->hasFormat("integer/songid")) {
        unsigned int dropRow;
        if (parent.row() >= 0) {
            dropRow = parent.row();
        } else if (row >= 0) {
            dropRow = row;
        } else {
            dropRow = m_singers.size();
        }
        emit songDroppedOnSinger(
                index(static_cast<int>(dropRow), 0).data(Qt::UserRole).toInt(),
                data->data("integer/songid").toInt(),
                static_cast<int>(dropRow)
        );
    }
    return false;
}

Qt::DropActions TableModelRotation::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags TableModelRotation::flags([[maybe_unused]]const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

uint TableModelRotation::singerTurnDistance(const int singerId) {
    const auto &singer = getSinger(singerId);
    const auto &curSinger = getSinger(m_currentSingerId);
    if (singer.position > curSinger.position)
        return singer.position - curSinger.position;
    else if (singer.position < curSinger.position)
        return (m_singers.size() - curSinger.position) + singer.position;
    return 0;
}

void TableModelRotation::setRotationTopSingerId(const int id) {
    m_rotationTopSingerId = id;
    m_settings.setLastRunRotationTopSingerId(id);
}

const RotationSinger& TableModelRotation::getSingerAtPosition(int position) const {
    if (position < 0 || position > m_singers.size() -1)
        return InvalidSinger;
    return m_singers.at(position);
}

const RotationSinger& TableModelRotation::getSinger(int singerId) const {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId] (const RotationSinger &singer) {
        return singer.id == singerId;
    });
    if (it == m_singers.end())
        return InvalidSinger;
    return *it;
}

const RotationSinger &TableModelRotation::getSingerByName(const QString &name) const {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name] (const RotationSinger &singer) {
        return singer.name == name;
    });
    if (it == m_singers.end())
        return InvalidSinger;
    return *it;
}

size_t TableModelRotation::singerCount() {
    return m_singers.size();
}
