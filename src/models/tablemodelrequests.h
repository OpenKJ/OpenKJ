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

#ifndef REQUESTSTABLEMODEL_H
#define REQUESTSTABLEMODEL_H

#include <QAbstractTableModel>
#include "okjsongbookapi.h"
#include <QIcon>
#include "settings.h"
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

class Request
{

private:
    int m_requestId;
    int m_timeStamp;
    QString m_artist;
    QString m_title;
    QString m_singer;
    int m_key;

public:
    Request(int RequestId, const QString &Singer, const QString &Artist, const QString &Title, int ts, int key = 0);
    [[nodiscard]] int requestId() const;
    [[nodiscard]] int timeStamp() const;
    [[nodiscard]] QString artist() const;
    void setArtist(const QString &artist);
    [[nodiscard]] QString title() const;
    void setTitle(const QString &title);
    [[nodiscard]] QString singer() const;
    void setSinger(const QString &singer);
    [[nodiscard]] int key() const;
};

class TableModelRequests : public QAbstractTableModel
{
    Q_OBJECT

private:
    std::string m_loggingPrefix{"[RequestsModel]"};
    std::shared_ptr<spdlog::logger> m_logger;
    QList<Request> m_requests;
    QIcon delete16;
    QIcon delete22;
    OKJSongbookAPI &songbookApi;
    Settings m_settings;

public:
    explicit TableModelRequests(OKJSongbookAPI &songbookAPI, QObject *parent = nullptr);
    enum {SINGER=0,ARTIST,TITLE,TIMESTAMP,KEYCHG};
    int count();
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    QList<Request> requests() {return m_requests; }

private slots:
    void requestsChanged(const OkjsRequests& requests);
};

#endif // REQUESTSTABLEMODEL_H
