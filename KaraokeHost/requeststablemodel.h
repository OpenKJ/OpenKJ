/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include <QTimer>
#include <QTime>


class Request
{

private:
    int m_requestId;
    int m_timeStamp;
    QString m_artist;
    QString m_title;
    QString m_singer;

public:
    Request(int RequestId, QString Singer, QString Artist, QString Title, int ts);
    int requestId() const;
    void setRequestId(int requestId);
    int timeStamp() const;
    void setTimeStamp(int timeStamp);
    QString artist() const;
    void setArtist(const QString &artist);
    QString title() const;
    void setTitle(const QString &title);
    QString singer() const;
    void setSinger(const QString &singer);

};

class RequestsTableModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    QTimer *timer;
    QList<Request> requests;
    QNetworkAccessManager *networkManager;
    int curSerial;
    QTime m_lastUpdate;
    bool m_connectionReset;
    bool m_delayWarningShown;
    bool m_clearingCache;

public:
    explicit RequestsTableModel(QObject *parent = 0);
    enum {REQUESTID=0,SINGER,ARTIST,TITLE,TIMESTAMP};
    void deleteAll();
    void deleteRequestId(int requestId);
    int count();
    QTime lastUpdate();
    void forceFullUpdate();
    void getAccepting();
    void setAccepting(bool accepting);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void updateReceived(QTime);
    void authenticationError();
    void sslError();
    void delayError(int);
    void acceptingReceived(bool);

private slots:
    void timerExpired();
    void onNetworkReply(QNetworkReply* reply);
    void onSslErrors(QNetworkReply * reply);
    void setAuth(QNetworkReply * reply, QAuthenticator * authenticator);

};

#endif // REQUESTSTABLEMODEL_H
