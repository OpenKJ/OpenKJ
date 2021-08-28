/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tablemodelrotation.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QSvgRenderer>
#include <QMimeData>
#include <QJsonArray>
#include <QJsonDocument>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <chrono>

std::ostream& operator<<(std::ostream& os, const QString& s);

TableModelRotation::TableModelRotation(QObject *parent)
        : QAbstractTableModel(parent) {
    m_logger = spdlog::get("logger");
    resizeIconsForFont(m_settings.applicationFont());
    m_rotationTopSingerId = m_settings.lastRunRotationTopSingerId();
}

QVariant TableModelRotation::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::SizeHintRole && orientation == Qt::Horizontal) {
        switch (section) {
            case COL_REGULAR:
            case COL_DELETE:
            case COL_ID: {
                int fHeight = QFontMetrics(m_settings.applicationFont()).height();
                return QSize(fHeight * 2,fHeight);
            }
            default:
                return {};
        }
    }
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
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
    switch (role) {
        case Qt::ToolTipRole:
            return getTooltipData(index);
        case Qt::UserRole:
            return m_singers.at(index.row()).id;
        case Qt::SizeHintRole:
            return getColumnSizeHint(index);
        case Qt::DecorationRole:
            return getDecorationRole(index);
        case Qt::TextAlignmentRole:
            if (index.column() == COL_ID)
                return Qt::AlignCenter;
            return {};
        case Qt::BackgroundRole:
            return getBackgroundRole(index);
        case Qt::ForegroundRole:
            if (m_singers.at(index.row()).id == m_currentSingerId && index.column() > 0)
                return QColor("black");
        case Qt::DisplayRole:
            return getDisplayData(index);
        default:
            return {};
    }
}

QVariant TableModelRotation::getBackgroundRole(const QModelIndex &index) const {
    if (m_singers.at(index.row()).id == m_currentSingerId) {
        if (index.column() > 0)
            return (m_settings.theme() == 1) ? QColor(180, 180, 0) : QColor("yellow");
    } else if (index.column() == COL_NAME) {
        const auto &singer = m_singers.at(index.row());
        if (singer.id == m_rotationTopSingerId && m_settings.rotationAltSortOrder())
            return QColor("green");
        if (singer.numSongsSung() == 0)
            return QColor(140, 30, 150);
    }
    return {};
}

QVariant TableModelRotation::getDecorationRole(const QModelIndex &index) const {
    if (index.column() == COL_NAME) {
        if (getSinger(index.data(Qt::UserRole).toInt()).numSongsUnsung() > 0)
            return m_iconGreenCircle;
        return m_iconYellowCircle;
    }
    return {};
}

QVariant TableModelRotation::getColumnSizeHint(const QModelIndex &index) const {
    switch (index.column()) {
        case COL_REGULAR:
        case COL_DELETE:
        case COL_ID: {
            auto fHeight = QFontMetrics(m_settings.applicationFont()).height();
            return QSize(fHeight, fHeight);
        }
        default:
            return {};
    }
}

QVariant TableModelRotation::getDisplayData(const QModelIndex &index) const {
    switch (index.column()) {
        case COL_ID:
            if (m_settings.rotationDisplayPosition())
                return positionTurnDistance(index.row());
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
                return m_singers.at(index.row()).nextSongArtistTitle();
            return {};
        default:
            return {};
    }
}

QVariant TableModelRotation::getTooltipData(const QModelIndex &index) const {
    QString toolTipText;
    int totalWaitDuration = 0;
    const auto &singer = m_singers.at(index.row());
    QString qSongsSung = QString::number(singer.numSongsSung());
    QString qSongsUnsung = QString::number(singer.numSongsUnsung());
    QString singerDistance = QString::number(positionTurnDistance(index.row()));
    if (m_currentSingerId == singer.id) {
        toolTipText = "Current singer - Sung: " + qSongsSung + " - Unsung: " + qSongsUnsung;
    } else {
        toolTipText = "Wait: " + singerDistance + " - Sung: " + qSongsSung + " - Unsung: " + qSongsUnsung;
        totalWaitDuration = positionWaitTime(singer.position);
    }
    toolTipText += "\nTime Added: " + m_singers.at(index.row()).addTs.toString("h:mm a");
    if (totalWaitDuration > 0) {
        toolTipText += "\n" + getWaitTimeString(totalWaitDuration);
    }
    return QString(toolTipText);
}

QString TableModelRotation::getWaitTimeString(int totalWaitDuration) {
    QString output;
    int minutes = totalWaitDuration / 60;
    int seconds = totalWaitDuration % 60;
    if (seconds > 0)
        minutes++;
    if (minutes > 60) {
        int hours = minutes / 60;
        minutes = minutes % 60;
        if (hours > 1)
            output += "Est wait time: " + QString::number(hours) + " hours " + QString::number(minutes) +
                      " min";
        else
            output +=
                    "Est wait time: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
    } else
        output += "Est wait time: " + QString::number(minutes) + " min";
    return output;
}

void TableModelRotation::loadData() {
    m_logger->debug("{} loading rotation data from DB on disk", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    m_singers.clear();
    QSqlQuery query;
    query.exec("SELECT singerid,name,position,regular,addts FROM rotationsingers ORDER BY position");
    if (auto sqlError = query.lastError(); sqlError.type() != QSqlError::NoError)
        m_logger->error("{} TableModelRotation - SQL error on load: {}", m_loggingPrefix,
                        sqlError.text());
    while (query.next()) {
        m_singers.emplace_back(okj::RotationSinger{
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
    m_logger->trace("{} [{}] Called", m_loggingPrefix, __func__);
    auto st = std::chrono::high_resolution_clock::now();

    m_logger->debug("{} Committing db changes to disk", m_loggingPrefix);
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM rotationsingers");
    query.prepare(
            "INSERT INTO rotationsingers (singerid,name,position,regular,regularid,addts) VALUES(:singerid,:name,:pos,:regular,:regularid,:addts)");
    for (const auto &singer: m_singers) {
        query.bindValue(":singerid", singer.id);
        query.bindValue(":name", singer.name);
        query.bindValue(":pos", singer.position);
        query.bindValue(":regular", singer.regular);
        query.bindValue(":regularid", -1);
        query.bindValue(":addts", singer.addTs);
        query.exec();
    }
    query.exec("COMMIT");
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} Commit error! Unable to write rotation changes to db on disk! Error: {}", m_loggingPrefix,
                        lastError.text());
    else
        m_logger->debug("{} Commit completed successfully", m_loggingPrefix);

    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
}

int TableModelRotation::singerAdd(const QString &name, const int positionHint) {
    m_logger->trace("{} [{}] Called with ({}, {})", m_loggingPrefix, __func__, name, positionHint);
    auto st = std::chrono::high_resolution_clock::now();

    m_logger->debug("{} Adding singer {} to rotation using positionHint {}", m_loggingPrefix, name, positionHint);
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
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error(
                "{} Commit error! Unable to write rotation changes to db on disk while adding singer! Error: {}",
                m_loggingPrefix, lastError.text());
    int singerId = query.lastInsertId().toInt();
    if (m_singers.empty()) {
        m_rotationTopSingerId = singerId;
        m_settings.setLastRunRotationTopSingerId(singerId);
    }
    if (singerId == -1) {
        m_logger->critical("{} Error occurred while inserting singer into the database singers table!!!",
                           m_loggingPrefix);
        return -1;
    }
    emit layoutAboutToBeChanged();
    m_singers.emplace_back(okj::RotationSinger{
            singerId,
            name,
            addPos,
            false,
            curTs
    });
    emit layoutChanged();
    bool singerMoved{false};
    int curSingerPos = getSinger(m_currentSingerId).position;
    switch (positionHint) {
        case ADD_FAIR: {
            if (curSingerPos > 0 && !m_settings.rotationAltSortOrder()) {
                singerMove(addPos, curSingerPos);
                singerMoved = true;
            }
            break;
        }
        case ADD_NEXT:
            if (curSingerPos != m_singers.size() - 2) {
                singerMove(addPos, curSingerPos + 1);
                singerMoved = true;
            }
            break;
        case ADD_BOTTOM:
            if (m_settings.rotationAltSortOrder()) {
                if (auto rotTopSingerPos = getSinger(m_rotationTopSingerId).position; rotTopSingerPos > 0) {
                    singerMove(addPos, rotTopSingerPos);
                    singerMoved = true;
                }
            }
        default:
            break;
    }

    if (!singerMoved)
        emit rotationModified();
    m_logger->debug("{} Singer add completed", m_loggingPrefix);
    outputRotationDebug();

    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );

    return singerId;
}

void TableModelRotation::singerMove(const int oldPosition, const int newPosition, const bool skipCommit) {
    m_logger->trace("{} [singerMove] Called with ({}, {}, {})", m_loggingPrefix, oldPosition, newPosition, skipCommit);
    auto st = std::chrono::high_resolution_clock::now();
    if (oldPosition == newPosition)
        return;
    if (auto singer = getSingerAtPosition(oldPosition); singer.isValid())
        m_logger->debug("{} Moving singer - Name: {} - Old position: {} - New position: {} - Skip DB commit: {}",
                        m_loggingPrefix, singer.name, oldPosition, newPosition, skipCommit);
    else
        m_logger->error("{} Error loading singer by position!!");
    emit layoutAboutToBeChanged();
    if (oldPosition > newPosition) {
        // moving up
        for (auto &singer: m_singers) {
            if (singer.position == oldPosition)
                singer.position = newPosition;
            else if (singer.position >= newPosition && singer.position < oldPosition)
                singer.position++;
        }
    } else {
        // moving down
        for (auto &singer: m_singers) {
            if (singer.position == oldPosition)
                singer.position = newPosition;
            else if (singer.position > oldPosition && singer.position <= newPosition)
                singer.position--;
        }
    }
    std::sort(m_singers.begin(), m_singers.end(), [](okj::RotationSinger &a, okj::RotationSinger &b) {
        return (a.position < b.position);
    });
    if (!skipCommit)
        commitChanges();

    emit layoutChanged();

    auto emitsSt = std::chrono::high_resolution_clock::now();

    // skipping this here because functions that use it emit rotationModified() themselves.
    if (!skipCommit)
        emit rotationModified();
    m_logger->trace("{} [singerMove] emits finished in {}ms",
                    m_loggingPrefix,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - emitsSt).count()
    );


    m_logger->debug("{} Singer move completed.", m_loggingPrefix);
    outputRotationDebug();
    m_logger->trace("{} [singerMove] finished in {}ms",
                    m_loggingPrefix,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
                    );
}

void TableModelRotation::singerSetName(const int singerId, const QString &newName) {
    m_logger->debug("{} Renaming singer '{}' to '{}'", m_loggingPrefix, getSinger(singerId).name, newName);
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId](okj::RotationSinger &singer) {
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
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} DB error! Unable to write rotation changes to db on disk! Error: {}", m_loggingPrefix,
                        lastError.text());
    emit rotationModified();
    outputRotationDebug();
}

void TableModelRotation::singerDelete(const int singerId) {
    m_logger->debug("{} Deleting singer id: {} name: {}", m_loggingPrefix, singerId, getSinger(singerId).name);
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
    auto it = std::remove_if(m_singers.begin(), m_singers.end(), [&singerId](okj::RotationSinger &singer) {
        return (singer.id == singerId);
    });
    m_singers.erase(it, m_singers.end());
    int pos{0};
    for (auto &singer: m_singers) {
        singer.position = pos++;
    }
    emit layoutChanged();
    emit rotationModified();
    commitChanges();
    outputRotationDebug();
}

bool TableModelRotation::singerExists(const QString &name) const {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name](const okj::RotationSinger &singer) {
        return (name.toLower() == singer.name.toLower());
    });
    return (it != m_singers.end());
}

void TableModelRotation::singerSetRegular(const int singerId, const bool isRegular) {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId](okj::RotationSinger &singer) {
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
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError) {
        m_logger->error("{} DB error! Unable to write rotation changes to db on disk! Error: {}", m_loggingPrefix,
                        lastError.text());
    }
}

void TableModelRotation::singerMakeRegular(const int singerId) {
    singerSetRegular(singerId, true);
}

void TableModelRotation::singerDisableRegularTracking(const int singerId) {
    singerSetRegular(singerId, false);
}

bool TableModelRotation::historySingerExists(const QString &name) const {
    auto hSingers = historySingers();
    auto it = std::find_if(hSingers.begin(), hSingers.end(), [&name](QString &hSinger) {
        return (name.toLower() == hSinger.toLower());
    });
    return (it != hSingers.end());
}

QStringList TableModelRotation::singers() const {
    QStringList names;
    names.reserve(static_cast<int>(m_singers.size()));
    for (const auto &singer: m_singers) {
        names.push_back(singer.name);
    }
    return names;
}

QStringList TableModelRotation::historySingers() const {
    QStringList names;
    QSqlQuery query;
    query.exec("SELECT name FROM historySingers");
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} DB error! Unable to read history singers data from the db on disk! Error: {}",
                        m_loggingPrefix, lastError.text());
    while (query.next())
        names << query.value(0).toString();

    return names;
}

int TableModelRotation::rotationDuration() const {
    int secs = 0;
    for (const auto &singer: m_singers) {
        secs += singer.nextSongDurationSecs();
    }
    return secs;
}

void TableModelRotation::clearRotation() {
    m_logger->debug("{} Clearing rotation", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    QSqlQuery query;
    query.exec("DELETE from queuesongs");
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} DB error! Error occurred while clearing the queuesongs db table on disk! Error: {}",
                        m_loggingPrefix, lastError.text());
    query.exec("DELETE FROM rotationsingers");
    if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
        m_logger->error("{} DB error! Error occurred while clearing the rotation singers db table on disk! Error: {}",
                        m_loggingPrefix, lastError.text());
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
    m_logger->debug("{} Setting singer id: {} name: '{}' as the current rotation singer", m_loggingPrefix,
                    currentSingerId,
                    getSinger(currentSingerId).name);
    emit layoutAboutToBeChanged();
    m_currentSingerId = currentSingerId;
    emit rotationModified();
    emit layoutChanged();
    m_settings.setCurrentRotationPosition(currentSingerId);
}

void TableModelRotation::outputRotationDebug() {
    m_logger->debug("{} -- Rotation debug output --", m_loggingPrefix);
    m_logger->debug("{} singerid    position    regular    added                name", m_loggingPrefix);
    int expectedPosition = 0;
    bool needsRepair = false;
    for (const auto &singer: m_singers) {
        m_logger->debug("{}      {:03}         {:03}     {:5}     {}    {}", m_loggingPrefix, singer.id,
                        singer.position, singer.regular, singer.addTs.toString("dd.MM.yy hh:mm:ss"),
                        singer.name);
        if (singer.position != expectedPosition) {
            needsRepair = true;
            m_logger->critical("{} ERROR DETECTED!!! - Singer position does not match expected position!",
                               m_loggingPrefix);
        }
        expectedPosition++;
    }
    m_logger->debug("{} -- Rotation debug output end --", m_loggingPrefix);
    if (needsRepair)
        fixSingerPositions();
}

void TableModelRotation::fixSingerPositions() {
    m_logger->error("{} Attempting to recover from corrupted rotation data", m_loggingPrefix);
    emit layoutAboutToBeChanged();
    int pos{0};
    for (auto &singer: m_singers) {
        singer.position = pos++;
    }
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
    if (indexes.isEmpty())
        return mimeData;
    mimeData->setData("integer/rotationpos",
                      indexes.at(0).sibling(indexes.at(0).row(), COL_POSITION).data().toByteArray().data());
    QJsonArray jArr;
    for (const auto &index: indexes) {
        // only act on one column to avoid duplicate singerIds in the list for each col
        if (index.column() != COL_NAME)
            continue;
        jArr.append(index.data(Qt::UserRole).toInt());
    }
    mimeData->setData("application/rotsingers", QJsonDocument(jArr).toJson());
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
        for (const auto &val: ids) {
            singerMove(getSinger(val.toInt()).position, static_cast<int>(dropRow), true);
        }
        commitChanges();
        emit rotationModified();
        if (dropRow == m_singers.size() - 1) {
            // moving to bottom
            emit singersMoved(static_cast<int>(m_singers.size() - ids.size()), 0,
                              static_cast<int>(m_singers.size() - 1), columnCount(QModelIndex()) - 1);
        } else if (getSinger(ids.at(0).toInt()).position < dropRow) {
            // moving down
            emit singersMoved(static_cast<int>(dropRow - ids.size() + 1), 0, static_cast<int>(dropRow),
                              columnCount(QModelIndex()) - 1);
        } else {
            // moving up
            emit singersMoved(static_cast<int>(dropRow), 0, static_cast<int>(dropRow + ids.size() - 1),
                              columnCount(QModelIndex()) - 1);
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

int TableModelRotation::singerTurnDistance(const int singerId) const {
    return positionTurnDistance(getSinger(singerId).position);
}

void TableModelRotation::setRotationTopSingerId(const int id) {
    m_rotationTopSingerId = id;
    m_settings.setLastRunRotationTopSingerId(id);
}

const okj::RotationSinger &TableModelRotation::getSingerAtPosition(int position) const {
    if (position < 0 || position > m_singers.size() - 1)
        return InvalidSinger;
    return m_singers.at(position);
}

const okj::RotationSinger &TableModelRotation::getSinger(int singerId) const {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&singerId](const okj::RotationSinger &singer) {
        return singer.id == singerId;
    });
    if (it == m_singers.end())
        return InvalidSinger;
    return *it;
}

const okj::RotationSinger &TableModelRotation::getSingerByName(const QString &name) const {
    auto it = std::find_if(m_singers.begin(), m_singers.end(), [&name](const okj::RotationSinger &singer) {
        return singer.name == name;
    });
    if (it == m_singers.end())
        return InvalidSinger;
    return *it;
}

size_t TableModelRotation::singerCount() {
    return m_singers.size();
}

int TableModelRotation::positionTurnDistance(int position) const {
    const auto &curSinger = getSinger(m_currentSingerId);
    if (position > curSinger.position)
        return position - curSinger.position;
    else if (position < curSinger.position)
        return static_cast<int>(m_singers.size() - curSinger.position) + position;
    return 0;
}

int TableModelRotation::positionWaitTime(int position) const {
    if (position < 0 || position > m_singers.size() - 1)
        return 0;

    const auto &curSinger = getSinger(m_currentSingerId);
    const auto &singer = getSingerAtPosition(position);
    int totalWaitDuration{0};

    if (curSinger.position == singer.position)
        return 0;

    if (curSinger.position < singer.position) {
        for (int i = curSinger.position; i < singer.position; i++) {
            const auto &loopSinger = getSingerAtPosition(i);
            if (i == curSinger.position) {
                totalWaitDuration += m_remainSecs;
            } else if (loopSinger.id != singer.id) {
                int nextDuration = loopSinger.nextSongDurationSecs();
                totalWaitDuration += nextDuration;
            }
        }
        return totalWaitDuration;
    }

    if (curSinger.position > singer.position) {
        for (int i = 0; i < singer.position; i++) {
            const auto &loopSinger = getSingerAtPosition(i);
            if (loopSinger.id != singer.id) {
                int nextDuration = loopSinger.nextSongDurationSecs();
                totalWaitDuration += nextDuration;
            }
        }
        for (int i = curSinger.position; i < m_singers.size(); i++) {
            const auto &loopSinger = getSingerAtPosition(i);
            if (i == curSinger.position)
                totalWaitDuration += 240;
            else if (loopSinger.id != singer.id) {
                int nextDuration = loopSinger.nextSongDurationSecs();
                totalWaitDuration += nextDuration;
            }
        }
        return totalWaitDuration;
    }

    return 0;
}
