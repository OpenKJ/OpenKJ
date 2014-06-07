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
#include "khsettings.h"


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
    connect(networkManager, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), this, SLOT(setAuth(QNetworkReply*,QAuthenticator*)));
}

void RequestsTableModel::timerExpired()
{
    if (settings->requestServerEnabled())
    {
        qDebug() << "RequestsClient -" << QTime::currentTime().toString() << " - Sending request for current serial";
        QUrl url(settings->requestServerUrl() + "/getSerial.php");
        QNetworkRequest request;
        request.setUrl(url);
        networkManager->get(request);
    }
}

void RequestsTableModel::onNetworkReply(QNetworkReply *reply)
{
    if (settings->requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
        //output some meaningful error msg
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);

    int recordType = json.object().value("recordtype").toDouble();
    int serial = json.object().value("serial").toDouble();
    qDebug() << "RequestsClient -" << QTime::currentTime().toString() <<  " - Received reply from server";
    m_lastUpdate = QTime::currentTime();
    emit updateReceived(m_lastUpdate);
    if (recordType == 0)
    {
        //serial only
        if (curSerial != serial)
        {
            qDebug() << "RequestsClient - Received serial - " << curSerial << " != " << serial << " - Serial mismatch.  Requesting full list.";
            QUrl url(settings->requestServerUrl() + "/getRequests.php");
            QNetworkRequest request;
            request.setUrl(url);
            networkManager->get(request);
        }
        else
        {
            qDebug() << "RequestsClient - Received serial - " << serial << " - Serials match.";
        }
    }
    else if (recordType == 1)
    {
        qDebug() << "RequestsClient - Recieved full list";
        curSerial = serial;
        QJsonArray reqArray = json.object().value("requests").toArray();
        emit layoutAboutToBeChanged();
        requests.clear();
        for (int i=0; i < reqArray.size(); i++)
        {
            QString artist = reqArray.at(i).toObject().value("artist").toString();
            QString title = reqArray.at(i).toObject().value("title").toString();
            QString singer = reqArray.at(i).toObject().value("singer").toString();
            int index = reqArray.at(i).toObject().value("id").toDouble();
            int reqtime = reqArray.at(i).toObject().value("reqtime").toDouble();
            requests << Request(index,singer,artist,title,reqtime);
        }
        emit layoutChanged();
    }
    else if (recordType == 2)
    {
        curSerial = serial;
        int reqID = json.object().value("delreq").toDouble();
        qDebug() << "RequestsClient - Received delete for request " << reqID << " - Removing request";
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
        // Go ahead and force a full download.  Occasionally new requests are being missed if they're
        // entered at just the right time while deleting a request or clearing the requests.
        QUrl url(settings->requestServerUrl() + "/getRequests.php");
        QNetworkRequest request;
        request.setUrl(url);
        networkManager->get(request);
    }
    else if (recordType == 3)
    {
        emit layoutAboutToBeChanged();
        qDebug() << "RequestsClient - Received clear - clearing all requests - new serial " << serial;
        curSerial = serial;
        requests.clear();
        emit layoutChanged();
        // Go ahead and force a full download.  Occasionally new requests are being missed if they're
        // entered at just the right time while deleting a request or clearing the requests.
        QUrl url(settings->requestServerUrl() + "/getRequests.php");
        QNetworkRequest request;
        request.setUrl(url);
        networkManager->get(request);
    }
}

void RequestsTableModel::onSslErrors(QNetworkReply *reply)
{
    static QString lastUrl;
    static bool errorEmitted = false;
    if (lastUrl != settings->requestServerUrl())
        errorEmitted = false;
    if (settings->requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    else if (!errorEmitted)
    {
        emit sslError();
        errorEmitted = true;

    }
    lastUrl = settings->requestServerUrl();
}

void RequestsTableModel::setAuth(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    static bool firstTry = true;
    static bool errorSignalSent = false;
    static QString lastUser;
    static QString lastPass;
    static QString lastUrl;
    if ((lastUser != settings->requestServerUsername()) || (lastPass != settings->requestServerPassword()) || (lastUrl != settings->requestServerUrl()))
    {
        firstTry = true;
        errorSignalSent = false;
    }
    if ((!firstTry) && (!errorSignalSent))
    {
        emit authenticationError();
        errorSignalSent = true;
        return;
    }
    qDebug() << "RequestsClient - Received authentication request, sending username and password";
    authenticator->setUser(settings->requestServerUsername());
    authenticator->setPassword(settings->requestServerPassword());
    lastUser = settings->requestServerUsername();
    lastPass = settings->requestServerPassword();
    lastUrl = settings->requestServerUrl();
    firstTry = false;
}


int RequestsTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return requests.size();
}

int RequestsTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
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
    Q_UNUSED(orientation);
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
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void RequestsTableModel::deleteAll()
{
    qDebug() << "RequestsClient - " << QTime::currentTime().toString() << " - Requesting clear all";
    QUrl url(settings->requestServerUrl() + "/clearRequests.php");
    QNetworkRequest request;
    request.setUrl(url);
    networkManager->get(request);
}

void RequestsTableModel::deleteRequestId(int requestId)
{
    qDebug() << "RequestsClient - " << QTime::currentTime().toString() << " - Requesting delete for request id: " << requestId;
    QUrl url(settings->requestServerUrl() + "/delRequest.php?reqID=" + QString::number(requestId));
    QNetworkRequest request;
    request.setUrl(url);
    networkManager->get(request);
}

int RequestsTableModel::count()
{
    return requests.count();
}

QTime RequestsTableModel::lastUpdate()
{
    return m_lastUpdate;
}

void RequestsTableModel::forceFullUpdate()
{
    QUrl url(settings->requestServerUrl() + "/getRequests.php");
    QNetworkRequest request;
    request.setUrl(url);
    networkManager->get(request);
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




