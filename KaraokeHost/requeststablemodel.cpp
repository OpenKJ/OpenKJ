#include "requeststablemodel.h"
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QDateTime>
#include <khsettings.h>


extern KhSettings *settings;

RequestsTableModel::RequestsTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    networkManager = new QNetworkAccessManager(this);
    curSerial = -1;
    timer = new QTimer(this);
    timer->setInterval(10000);
    timer->start();
    if (settings->requestServerEnabled())
        timerExpired();
    connect(timer, SIGNAL(timeout()), this, SLOT(timerExpired()));
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    connect(networkManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*)));
}

void RequestsTableModel::timerExpired()
{
    if (settings->requestServerEnabled())
    {
        qDebug() << "Timer tick";
        QUrl url(settings->requestServerUrl() + "/getSerial.php");
        QNetworkRequest request;
        request.setUrl(url);
        networkManager->get(request);
    }
}

void RequestsTableModel::onNetworkReply(QNetworkReply *reply)
{
    reply->ignoreSslErrors();
    if (reply->error() != QNetworkReply::NoError)
    {
        if (reply->error() != QNetworkReply::SslHandshakeFailedError)
        {
        qDebug() << reply->errorString();
        //output some meaningful error msg
        return;
        }
    }
    QByteArray data = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);

    int recordType = json.object().value("recordtype").toDouble();
    int serial = json.object().value("serial").toDouble();
    qDebug() << "Serial #" << serial;
    if (recordType == 0)
    {
        //serial only
        if (curSerial != serial)
        {
            qDebug() << "Serial only pull - " << curSerial << " != " << serial << " - Serial mismatch.  Downloading full list.";
            QUrl url(settings->requestServerUrl() + "/getRequests.php");
            QNetworkRequest request;
            request.setUrl(url);
            networkManager->get(request);
        }
        else
        {
            qDebug() << "Serial only pull - " << serial << " - Serials match.";
        }
    }
    else if (recordType == 1)
    {
        //full pull
        qDebug() << "Full data pull - updating - serial " << serial;
        curSerial = serial;
        int count = json.object().value("numreqs").toDouble();
        QJsonArray reqArray = json.object().value("requests").toArray();
        qDebug() << "Requests: " << count << " Serial no: " << serial;
        //requestsModel->clear();
        emit layoutAboutToBeChanged();
        requests.clear();
        for (unsigned int i=0; i < reqArray.size(); i++)
        {
            QString artist = reqArray.at(i).toObject().value("artist").toString();
            QString title = reqArray.at(i).toObject().value("title").toString();
            QString singer = reqArray.at(i).toObject().value("singer").toString();
            int index = reqArray.at(i).toObject().value("id").toDouble();
            int reqtime = reqArray.at(i).toObject().value("reqtime").toDouble();
            //requestsModel->addRequest(index, singer, artist, title, reqtime);
            requests << Request(index,singer,artist,title,reqtime);
        }
        emit layoutChanged();
    }
    else if (recordType == 2)
    {
        qDebug() << "Deleted request - removing item - new serial " << serial;
        curSerial = serial;
        int reqID = json.object().value("delreq").toDouble();
        int delIndex = -1;
        for (int i=0; i < requests.size(); i++)
        {
            if (requests.at(i).requestId() == reqID)
                delIndex = i;
        }
        if (delIndex != -1)
        {
            emit layoutAboutToBeChanged();
            requests.removeAt(delIndex);
            emit layoutChanged();
        }
    }
    else if (recordType == 3)
    {
        emit layoutAboutToBeChanged();
        qDebug() << "Clear request - clearing requests - new serial " << serial;
        curSerial = serial;
        requests.clear();
        emit layoutChanged();
    }
}

void RequestsTableModel::onSslErrors(QNetworkReply *reply)
{
    reply->ignoreSslErrors();
}


int RequestsTableModel::rowCount(const QModelIndex &parent) const
{
    return requests.size();
}

int RequestsTableModel::columnCount(const QModelIndex &parent) const
{
    return 6;
}

QVariant RequestsTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(index.row() >= requests.size() || index.row() < 0)
        return QVariant();
    if ((index.column() == 5) && (role == Qt::DecorationRole))
    {
        QPixmap icon(":/icons/Icons/edit-delete.png");
        return icon;
    }
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case REQUESTID:
            return QString::number(requests.at(index.row()).requestId());
        case SINGER:
            return requests.at(index.row()).singer();
        case ARTIST:
            return requests.at(index.row()).artist();
        case TITLE:
            return requests.at(index.row()).title();
        case TIMESTAMP:
            QDateTime ts;
            ts.setTime_t(requests.at(index.row()).timeStamp());
            return ts.toString("M-d-yy h:mm ap");
        }
    }
    return QVariant();
}

QVariant RequestsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        switch(section) {
        case REQUESTID:
            return "RequestID";
        case SINGER:
            return "Singer";
        case ARTIST:
            return "Artist";
        case TITLE:
            return "Title";
        case TIMESTAMP:
            return "Received";
        }
    }
    return QVariant();
}

Qt::ItemFlags RequestsTableModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void RequestsTableModel::deleteAll()
{
    QUrl url(settings->requestServerUrl() + "/clearRequests.php");
    QNetworkRequest request;
    request.setUrl(url);
    networkManager->get(request);
}

void RequestsTableModel::deleteRequestId(int requestId)
{
    QUrl url(settings->requestServerUrl() + "/delRequest.php?reqID=" + QString::number(requestId));
    QNetworkRequest request;
    request.setUrl(url);
    networkManager->get(request);
}

int RequestsTableModel::count()
{
    return requests.count();
}

Request::Request(int RequestId, QString Singer, QString Artist, QString Title, int ts)
{
    m_requestId = RequestId;
    m_singer = Singer;
    m_artist = Artist;
    m_title = Title;
    m_timeStamp = ts;
}

int Request::requestId() const
{
    return m_requestId;
}

void Request::setRequestId(int requestId)
{
    m_requestId = requestId;
}
int Request::timeStamp() const
{
    return m_timeStamp;
}

void Request::setTimeStamp(int timeStamp)
{
    m_timeStamp = timeStamp;
}
QString Request::artist() const
{
    return m_artist;
}

void Request::setArtist(const QString &artist)
{
    m_artist = artist;
}
QString Request::title() const
{
    return m_title;
}

void Request::setTitle(const QString &title)
{
    m_title = title;
}
QString Request::singer() const
{
    return m_singer;
}

void Request::setSinger(const QString &singer)
{
    m_singer = singer;
}




