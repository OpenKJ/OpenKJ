#include "okjsongbookapi.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include "settings.h"

extern Settings *settings;

QDebug operator<<(QDebug dbg, const OkjsVenue &okjsvenue)
{
    dbg.nospace() << "venue_id: " << okjsvenue.venueId << " name: " << okjsvenue.name << " urlName: " << okjsvenue.urlName << " accepting: " << okjsvenue.accepting;
    return dbg.maybeSpace();
}


OKJSongbookAPI::OKJSongbookAPI(QObject *parent) : QObject(parent)
{
   // manager = new QNetworkAccessManager(this);
    refreshVenues();
    refreshRequests();
}

int OKJSongbookAPI::getSerial()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getSerial");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    QByteArray replyData = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(replyData);
    int serial = json.object().value("serial").toInt();
    delete(manager);
    return serial;
}

OkjsRequests OKJSongbookAPI::refreshRequests()
{
    QJsonObject jsonObject;
    jsonObject.insert("api_key", settings->requestServerApiKey());
    jsonObject.insert("command","getRequests");
    jsonObject.insert("venue_id", settings->requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    QByteArray replyData = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(replyData);
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
    delete(manager);
    return l_requests;
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
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    delete(manager);
}

bool OKJSongbookAPI::requestsEnabled()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getAccepting");
    mainObject.insert("venue_id", settings->requestServerVenue());
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    QByteArray replyData = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(replyData);
    bool accepting = json.object().value("accepting").toBool();
//    QFile tmpfile("/tmp/requestsreply.txt");
//    tmpfile.open(QFile::ReadWrite | QFile::Truncate);
//    tmpfile.write(replyData);
//    tmpfile.close();
    delete(manager);
    return accepting;
}

void OKJSongbookAPI::setRequestsEnabled(bool enabled)
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","setAccepting");
    mainObject.insert("venue_id", settings->requestServerVenue());
    mainObject.insert("accepting", enabled);
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    delete(manager);
}

void OKJSongbookAPI::setApiKey(QString apiKey)
{
    this->apiKey = apiKey;
}

OkjsVenues OKJSongbookAPI::refreshVenues()
{
    QJsonObject mainObject;
    mainObject.insert("api_key", settings->requestServerApiKey());
    mainObject.insert("command","getVenues");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl(settings->requestServerUrl()));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    QByteArray replyData = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(replyData);
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
      //venues = l_venues;
//    qWarning() << l_venues;
//    QFile tmpfile("/tmp/venuesreply.txt");
//    tmpfile.open(QFile::ReadWrite | QFile::Truncate);
//    tmpfile.write(replyData);
//    tmpfile.close();
    delete(manager);
    return l_venues;
}

OkjsVenues OKJSongbookAPI::getVenues()
{
    return refreshVenues();
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
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, jsonDocument.toJson());
    while (!reply->isFinished())
        QApplication::processEvents();
    delete(manager);
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
