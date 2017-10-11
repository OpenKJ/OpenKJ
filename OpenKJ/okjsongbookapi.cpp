#include "okjsongbookapi.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSqlQuery>
#include "settings.h"

extern Settings *settings;

QDebug operator<<(QDebug dbg, const OkjsVenue &okjsvenue)
{
    dbg.nospace() << "venue_id: " << okjsvenue.venueId << " name: " << okjsvenue.name << " urlName: " << okjsvenue.urlName << " accepting: " << okjsvenue.accepting;
    return dbg.maybeSpace();
}


OKJSongbookAPI::OKJSongbookAPI(QObject *parent) : QObject(parent)
{
    delayErrorEmitted = false;
    connectionReset = false;
    serial = 0;
    timer = new QTimer(this);
    timer->setInterval(10000);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    if (settings->requestServerEnabled())
        refreshVenues();
    timer->start();
}

void OKJSongbookAPI::getSerial()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getSerial");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::refreshRequests()
{
    QJsonObject jsonObject;
    jsonObject.insert("api_key", settings->requestServerApiKey());
    jsonObject.insert("command","getRequests");
    jsonObject.insert("venue_id", settings->requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::removeRequest(int requestId)
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","deleteRequest");
    mainObject.insert("venue_id", settings->requestServerVenue());
    mainObject.insert("request_id", requestId);
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

bool OKJSongbookAPI::getAccepting()
{
    for (int i=0; i<venues.size(); i++)
    {
        if (venues.at(i).venueId == settings->requestServerVenue())
            return venues.at(i).accepting;
    }
    return false;
}

void OKJSongbookAPI::setAccepting(bool enabled)
{
    qWarning() << "OKJSongbookAPI::setRequestsEnabled(" << enabled << ") called";
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","setAccepting");
    mainObject.insert("venue_id", settings->requestServerVenue());
    mainObject.insert("accepting", enabled);
    qWarning() << "Using values - venue_id: " << settings->requestServerVenue() << " - accepting: " << enabled;
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::refreshVenues(bool blocking)
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getVenues");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    if (blocking)
    {
        while (!reply->isFinished())
            QApplication::processEvents();
    }
}

void OKJSongbookAPI::clearRequests()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","clearRequests");
    mainObject.insert("venue_id", settings->requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::updateSongDb()
{
    emit remoteSongDbUpdateStart();
    int songsPerDoc = 1000;
    QList<QJsonDocument> jsonDocs;
    QSqlQuery query;
    int numEntries = 0;
    if (query.exec("SELECT COUNT(DISTINCT artist||title) FROM dbsongs"))
    {
        if (query.next())
            numEntries = query.value(0).toInt();
    }
    if (query.exec("SELECT DISTINCT artist,title FROM dbsongs ORDER BY artist ASC, title ASC"))
    {
        bool done = false;
        qWarning() << "Number of results: " << numEntries;
        int numDocs = numEntries / songsPerDoc;
        if (numEntries % songsPerDoc > 0)
            numDocs++;
        emit remoteSongDbUpdateNumDocs(numDocs);
        qWarning() << "Emitted remoteSongDbUpdateNumDocs(" << numDocs << ")";
        int docs = 0;
        while (!done)
        {
            QApplication::processEvents();
            QJsonArray songsArray;
            int count = 0;
            while ((query.next()) && (count < songsPerDoc))
            {
                QJsonObject songObject;
                songObject.insert("artist", query.value(0).toString());
                songObject.insert("title", query.value(1).toString());
                songsArray.insert(0, songObject);
                QApplication::processEvents();
                count++;
            }
            docs++;
            if (count < songsPerDoc)
                done = true;
            QJsonObject mainObject;
            mainObject.insert("api_key", settings->requestServerApiKey());
            mainObject.insert("command","addSongs");
            mainObject.insert("songs", songsArray);
            QJsonDocument jsonDocument;
            jsonDocument.setObject(mainObject);
            jsonDocs.append(jsonDocument);
        }
        QUrl url(settings->requestServerUrl());
        QJsonObject mainObject;
        mainObject.insert("api_key", settings->requestServerApiKey());
        mainObject.insert("command","clearDatabase");
        QJsonDocument jsonDocument;
        jsonDocument.setObject(mainObject);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
        while (!reply->isFinished())
            QApplication::processEvents();
        for (int i=0; i < jsonDocs.size(); i++)
        {
            QApplication::processEvents();
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            QNetworkReply *reply = manager->post(request, jsonDocs.at(i).toJson());
            while (!reply->isFinished())
                QApplication::processEvents();
            emit remoteSongDbUpdateProgress(i);
        }
    }
    emit remoteSongDbUpdateDone();
}

void OKJSongbookAPI::onSslErrors(QNetworkReply *reply, QList<QSslError> errors)
{
    Q_UNUSED(errors)
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

void OKJSongbookAPI::onNetworkReply(QNetworkReply *reply)
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
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    if (error)
    {
        qWarning() << "Got error json reply";
        qWarning() << "Error string: " << json.object().value("errorString");
        return;
    }
    if (command == "getSerial")
    {
        int newSerial = json.object().value("serial").toInt();
        if (newSerial == 0)
        {
            qWarning() << "Server didn't return valid serial";
            return;
        }
        if (serial == newSerial)
        {
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
            //qWarning() << "Got serial: " << newSerial << " - No Change";
        }
        else
        {
            qWarning() << "Got serial: " << newSerial << " - Changed, previous: " << serial;
            qWarning() << "Refreshing venues and requests";
            serial = newSerial;
            refreshRequests();
            refreshVenues();
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
        }
    }
    if (command == "getVenues")
    {
        qWarning() << "Venues received from server";
        QJsonArray venuesArray = json.object().value("venues").toArray();
        OkjsVenues l_venues;
        for (int i=0; i < venuesArray.size(); i++)
        {
            OkjsVenue venue;
            QJsonObject jsonObject = venuesArray.at(i).toObject();
            venue.venueId = jsonObject.value("venue_id").toInt();
            venue.name = jsonObject.value("name").toString();
            venue.urlName = jsonObject.value("url_name").toString();
            venue.accepting = jsonObject.value("accepting").toBool();
            l_venues.append(venue);
        }
        if (venues != l_venues)
        {
            venues = l_venues;
            emit venuesChanged(venues);
        }
    }
    if (command == "clearRequests")
    {
        qWarning() << "Requests cleared received from server";
        refreshRequests();
        refreshVenues();
    }
    if (command == "getRequests")
    {
        qWarning() << "Requests received from server";
        QJsonArray requestsArray = json.object().value("requests").toArray();
        OkjsRequests l_requests;
        for (int i=0; i < requestsArray.size(); i++)
        {
            OkjsRequest request;
            QJsonObject jsonObject = requestsArray.at(i).toObject();
            request.requestId = jsonObject.value("request_id").toInt();
            request.artist = jsonObject.value("artist").toString();
            request.title = jsonObject.value("title").toString();
            request.singer = jsonObject.value("singer").toString();
            request.time = jsonObject.value("request_time").toInt();
            l_requests.append(request);
        }
        if (requests != l_requests)
        {
            requests = l_requests;
            emit requestsChanged(requests);
        }
    }
    if (command == "setAccepting")
    {
        int venue_id = json.object().value("venue_id").toInt();
        bool accepting = json.object().value("accepting").toBool();
        qWarning() << "Server replied to setAccepting command. Venue: " << venue_id << " Accepting: " << accepting;
        qWarning() << "Requesting refresh of venues from server";
        refreshVenues();
        refreshRequests();
    }
    if (command == "deleteRequest")
    {
        qWarning() << "Server replied to deleteRequest command, refreshing requests";
        refreshRequests();
        refreshVenues();
    }
}

void OKJSongbookAPI::timerTimeout()
{
    if (settings->requestServerEnabled())
    {
        //qWarning() << "RequestsClient - Seconds since last update: " << lastSync.secsTo(QTime::currentTime());
        if ((lastSync.secsTo(QTime::currentTime()) > 300) && (!delayErrorEmitted))
        {
            emit delayError(lastSync.secsTo(QTime::currentTime()));
            delayErrorEmitted = true;
        }
        else if ((lastSync.secsTo(QTime::currentTime()) > 200) && (!connectionReset))
        {
            refreshRequests();
            refreshVenues();
            connectionReset = true;
        }
        else
        {
            connectionReset = false;
            delayErrorEmitted = false;
        }
        getSerial();
    }
}

bool OkjsVenue::operator ==(const OkjsVenue &v) const
{
    if (venueId != v.venueId)
        return false;
    if (name != v.name)
        return false;
    if (urlName != v.urlName)
        return false;
    if (accepting != v.accepting)
        return false;
    return true;
}

bool OkjsRequest::operator ==(const OkjsRequest r) const
{
    if (r.requestId != requestId)
        return false;
    if (r.artist != artist)
        return false;
    if (r.title != title)
        return false;
    if (r.time != time)
        return false;
    if (r.singer != singer)
        return false;
    return true;
}
