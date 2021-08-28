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

#include "tablemodelrequests.h"
#include <QDateTime>


TableModelRequests::TableModelRequests(OKJSongbookAPI &songbookAPI, QObject *parent) :
        QAbstractTableModel(parent),
        songbookApi(songbookAPI) {
    m_logger = spdlog::get("logger");
    connect(&songbookApi, &OKJSongbookAPI::requestsChanged, this, &TableModelRequests::requestsChanged);
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    delete16 = QIcon(thm + "actions/16/edit-delete.svg");
    delete22 = QIcon(thm + "actions/22/edit-delete.svg");
}

void TableModelRequests::requestsChanged(const OkjsRequests &requests) {
    emit layoutAboutToBeChanged();
    m_requests.clear();
    for (const auto &request : requests) {
        int index = request.requestId;
        QString singer = request.singer;
        QString artist = request.artist;
        QString title = request.title;
        int reqtime = request.time;
        int key = request.key;
        m_requests << Request(index, singer, artist, title, reqtime, key);
    }
    emit layoutChanged();
}

int TableModelRequests::rowCount(const QModelIndex &parent) const {
    return m_requests.size();
}

int TableModelRequests::columnCount(const QModelIndex &parent) const {
    return 6;
}

QVariant TableModelRequests::data(const QModelIndex &index, int role) const {
    QSize sbSize(QFontMetrics(m_settings.applicationFont()).height(), QFontMetrics(m_settings.applicationFont()).height());
    if (!index.isValid())
        return {};

    if (index.row() >= m_requests.size() || index.row() < 0)
        return {};
    if ((index.column() == 5) && (role == Qt::DecorationRole)) {
        if (sbSize.height() > 18)
            return delete22.pixmap(sbSize);
        else
            return delete16.pixmap(sbSize);
    }
    if (role == Qt::TextAlignmentRole)
        switch (index.column()) {
            case KEYCHG:
                return Qt::AlignCenter;
            default:
                return Qt::AlignLeft;
        }
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        switch (index.column()) {
            case SINGER:
                return m_requests.at(index.row()).singer();
            case ARTIST:
                return m_requests.at(index.row()).artist();
            case TITLE:
                return m_requests.at(index.row()).title();
            case KEYCHG:
                if (m_requests.at(index.row()).key() == 0)
                    return "";
                else if (m_requests.at(index.row()).key() > 0)
                    return "+" + QString::number(m_requests.at(index.row()).key());
                else
                    return QString::number(m_requests.at(index.row()).key());
            case TIMESTAMP:
                QDateTime ts;
                ts.setTime_t(m_requests.at(index.row()).timeStamp());
                return ts.toString("M-d-yy h:mm ap");
        }
    }
    if (role == Qt::UserRole)
        return m_requests.at(index.row()).requestId();
    return {};
}

QVariant TableModelRequests::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case SINGER:
                return "Singer";
            case ARTIST:
                return "Artist";
            case TITLE:
                return "Title";
            case KEYCHG:
                return "Key";
            case TIMESTAMP:
                return "Received";
            default:
                return "";
        }
    }
    return {};
}

Qt::ItemFlags TableModelRequests::flags(const QModelIndex &index) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TableModelRequests::count() {
    return m_requests.count();
}

int Request::key() const {
    return m_key;
}

Request::Request(int RequestId, const QString &Singer, const QString &Artist, const QString &Title, int ts, int key) {
    m_requestId = RequestId;
    m_singer = Singer;
    m_artist = Artist;
    m_title = Title;
    m_timeStamp = ts;
    m_key = key;
}

int Request::requestId() const {
    return m_requestId;
}

int Request::timeStamp() const {
    return m_timeStamp;
}

QString Request::artist() const {
    return m_artist;
}

void Request::setArtist(const QString &artist) {
    m_artist = artist;
}

QString Request::title() const {
    return m_title;
}

void Request::setTitle(const QString &title) {
    m_title = title;
}

QString Request::singer() const {
    return m_singer;
}

void Request::setSinger(const QString &singer) {
    m_singer = singer;
}




