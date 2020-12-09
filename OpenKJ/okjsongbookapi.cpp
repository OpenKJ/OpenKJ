#include "okjsongbookapi.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSqlQuery>
#include <QMessageBox>
#include <QPushButton>
#include "settings.h"
#include "idledetect.h"

extern Settings *settings;
extern IdleDetect *filter;

QDebug operator<<(QDebug dbg, const OkjsVenue &okjsvenue)
{
    dbg.nospace() << "venue_id: " << okjsvenue.venueId << " name: " << okjsvenue.name << " urlName: " << okjsvenue.urlName << " accepting: " << okjsvenue.accepting;
    return dbg.maybeSpace();
}


OKJSongbookAPI::OKJSongbookAPI(QObject *parent) : QObject(parent)
{
    programIsIdle = false;
    delayErrorEmitted = false;
    connectionReset = false;
    cancelUpdate = false;
    updateInProgress = false;
    serial = 0;
    entitledSystems = 1;
    timer = new QTimer(this);
    timer->setInterval(settings->requestServerInterval() * 1000);
    alertTimer = new QTimer(this);
    alertTimer->start(600000);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(alertTimer, SIGNAL(timeout()), this, SLOT(alertTimerTimeout()));
    connect(settings, SIGNAL(requestServerIntervalChanged(int)), this, SLOT(setInterval(int)));
    connect(filter, SIGNAL(idleStateChanged(bool)), this, SLOT(idleStateChanged(bool)));
    if (settings->requestServerEnabled())
    {
        getEntitledSystemCount();
        refreshVenues();
        alertCheck();
    }
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

void OKJSongbookAPI::triggerTestAdd()
{
    QJsonObject jsonObject;
    jsonObject.insert("command","testingAddRandomRequest");
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
    alertCheck();
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","setAccepting");
    mainObject.insert("venue_id", settings->requestServerVenue());
    mainObject.insert("accepting", enabled);
    mainObject.insert("system_id", settings->systemId());
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
    cancelUpdate = false;
    updateInProgress = true;
    emit remoteSongDbUpdateStart();
    int songsPerDoc = 1000;
    QList<QJsonDocument> jsonDocs;
    QSqlQuery query;
    int numEntries = 0;
    if (cancelUpdate)
        return;
    if (query.exec("SELECT COUNT(DISTINCT artist||title) FROM dbsongs WHERE discid != '!!DROPPED!!' AND discid != '!!BAD!!'"))
    {
        if (query.next())
            numEntries = query.value(0).toInt();
    }
    if (cancelUpdate)
        return;
    if (query.exec("SELECT DISTINCT artist,title FROM dbsongs WHERE discid != '!!DROPPED!!' AND discid != '!!BAD!!' ORDER BY artist ASC, title ASC"))
    {
        if (cancelUpdate)
            return;
        bool done = false;
        qInfo() << "Number of results: " << numEntries;
        int numDocs = numEntries / songsPerDoc;
        if (numEntries % songsPerDoc > 0)
            numDocs++;
        emit remoteSongDbUpdateNumDocs(numDocs);
        qInfo() << "Emitted remoteSongDbUpdateNumDocs(" << numDocs << ")";
        int docs = 0;
        while (!done)
        {
            if (cancelUpdate)
                return;
            QApplication::processEvents();
            QJsonArray songsArray;
            int count = 0;
            while ((query.next()) && (count < songsPerDoc))
            {
                if (cancelUpdate)
                    return;
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
            mainObject.insert("system_id", settings->systemId());
            QJsonDocument jsonDocument;
            jsonDocument.setObject(mainObject);
            jsonDocs.append(jsonDocument);
        }
        QUrl url(settings->requestServerUrl());
        QJsonObject mainObject;
        mainObject.insert("api_key", settings->requestServerApiKey());
        mainObject.insert("command","clearDatabase");
        mainObject.insert("system_id", settings->systemId());
        QJsonDocument jsonDocument;
        jsonDocument.setObject(mainObject);
        QNetworkRequest request(url);
        if (cancelUpdate)
            return;
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
        while (!reply->isFinished())
            QApplication::processEvents();
        qInfo() << reply->readAll();
        for (int i=0; i < jsonDocs.size(); i++)
        {
            if (cancelUpdate)
                return;
            QApplication::processEvents();
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            QNetworkReply *reply = manager->post(request, jsonDocs.at(i).toJson());
            while (!reply->isFinished()){
                if (cancelUpdate)
                    return;
                QApplication::processEvents();
            }
            if (cancelUpdate)
                return;
            emit remoteSongDbUpdateProgress(i + 1);
        }
    }
    if (cancelUpdate)
        return;
    updateInProgress = false;
    emit remoteSongDbUpdateDone();
}

bool OKJSongbookAPI::test()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getSerial");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkAccessManager m_NetworkMngr;

    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_NetworkMngr.post(request, jsonDocument.toJson());
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()),&loop, SLOT(quit()));
    loop.exec();
    if (reply->error() != QNetworkReply::NoError)
    {
        qInfo() << "Network error: " << reply->errorString();
        emit testFailed(reply->errorString());
        return false;
    }
    QByteArray data = reply->readAll();
    delete reply;
    QJsonDocument json = QJsonDocument::fromJson(data);
    qInfo() << json;
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    qInfo() << "error = " << error;
    if (json.object().value("errorString").toString() != "")
    {
        qInfo() << "Got error json reply";
        qInfo() << "Error string: " << json.object().value("errorString");
        emit testFailed(json.object().value("errorString").toString());
        return false;
    }
    if (command == "getSerial")
    {
        int newSerial = json.object().value("serial").toInt();
        if (newSerial != 0)
        {
            qInfo() << "SongbookAPI - Server returned good serial";
            emit testPassed();
            return true;
        }
    }
    qInfo() << data;
    emit testFailed("Unknown error");
    return false;
}

void OKJSongbookAPI::alertCheck()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getAlert");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
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

void OKJSongbookAPI::onTestSslErrors(QNetworkReply *reply, QList<QSslError> errors)
{
    if (settings->requestServerIgnoreCertErrors())
    {
        reply->ignoreSslErrors();
        return;
    }
    QString errorText;
    foreach (QSslError error, errors) {
        errorText += error.errorString() + "\n";
    }
    emit testSslError(errorText);
}

void OKJSongbookAPI::onNetworkReply(QNetworkReply *reply)
{
    if (settings->requestServerIgnoreCertErrors())
        reply->ignoreSslErrors();
    if (reply->error() != QNetworkReply::NoError)
    {
        qInfo() << reply->errorString();
        //output some meaningful error msg
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(data);
    QString command = json.object().value("command").toString();
    bool error = json.object().value("error").toBool();
    if (error)
    {
        qInfo() << "Got error json reply";
        qInfo() << "Error string: " << json.object().value("errorString");
        return;
    }
    if (command == "testingAddRandomRequest")
    {
        qInfo() << "Got reply from testingAddRandomRequest, refreshing requests";
        refreshRequests();
    }
    if (command == "getEntitledSystemCount")
    {
        qInfo() << json;
        entitledSystems = json.object().value("count").toInt();
        emit entitledSystemCountChanged(entitledSystems);
        qInfo() << "OKJSongbookAPI: Server reports entitled to run " << entitledSystems << " concurrent systems";
    }
    if (command == "getAlert")
    {
        if(json.object().value("alert").toBool())
        {
            emit alertRecieved(json.object().value("title").toString(), json.object().value("message").toString());
            venues.clear();
            refreshVenues();
        }
    }
    if (command == "getSerial")
    {
        int newSerial = json.object().value("serial").toInt();
        if (newSerial == 0)
        {
            qInfo() << "SongbookAPI - Server didn't return valid serial";
            return;
        }
        if (serial == newSerial)
        {
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
        }
        else
        {
            serial = newSerial;
            refreshRequests();
            refreshVenues();
            lastSync = QTime::currentTime();
            emit synchronized(lastSync);
        }
    }
    if (command == "getVenues")
    {
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
            getEntitledSystemCount();
        }
        lastSync = QTime::currentTime();
        emit synchronized(lastSync);
    }
    if (command == "clearRequests")
    {
        refreshRequests();
        refreshVenues();
    }
    if (command == "getRequests")
    {
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
            request.key = jsonObject.value("key_change").toInt();
            l_requests.append(request);
        }
        if (requests != l_requests)
        {
            requests = l_requests;
            emit requestsChanged(requests);
        }
        lastSync = QTime::currentTime();
        emit synchronized(lastSync);
    }
    if (command == "setAccepting")
    {
        refreshVenues();
        refreshRequests();
    }
    if (command == "deleteRequest")
    {
        refreshRequests();
        refreshVenues();
    }
}

void OKJSongbookAPI::timerTimeout()
{
    if (settings->requestServerEnabled() && !programIsIdle)
    {
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

void OKJSongbookAPI::alertTimerTimeout()
{
    if (settings->requestServerEnabled())
        alertCheck();
}

void OKJSongbookAPI::setInterval(int interval)
{
    timer->setInterval(interval * 1000);
}

void OKJSongbookAPI::idleStateChanged(bool isIdle)
{
    if (!isIdle)
    {
        // reset last update time to current to avoid showing
        // warning dialog to user due to updates being suppressed
        // during idle period
        lastSync = QTime::currentTime();
        timerTimeout();
    }
    programIsIdle = isIdle;
    qInfo() << "Program idle state changed to: " << isIdle;
}

void OKJSongbookAPI::getEntitledSystemCount()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getEntitledSystemCount");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(settings->requestServerUrl());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

void OKJSongbookAPI::dbUpdateCanceled()
{
    qInfo() << "SBAPI - dbUpdateCancelled() fired";
    if (!cancelUpdate && updateInProgress)
    {
        QMessageBox msgBox(nullptr);
        msgBox.setWindowTitle(tr("Cancelling Update"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Are you sure you want to cancel the Songbook DB update?\n\nYour previous Songbook DB contents have already been cleared.\n\nCancelling now will result in an incomplete database of songs on your Songbook account.\n");
   //     msgBox.setInformativeText("Are you sure?  Your previous Songbook DB contents have already been cleared.\nCancelling now will result in an incomplete database of songs on your Songbook account.");
        QPushButton *yesButton = msgBox.addButton(tr("Cancel Update"), QMessageBox::AcceptRole);
        msgBox.addButton(tr("Continue Update"), QMessageBox::RejectRole);
        msgBox.exec();
        if (msgBox.clickedButton() == yesButton)
        {
            cancelUpdate = true;
            updateInProgress = false;
        }
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

