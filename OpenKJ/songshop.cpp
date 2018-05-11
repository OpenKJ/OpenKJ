#include "songshop.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFileInfo>
#include "settings.h"
#include <QDir>

extern Settings *settings;

SongShop::SongShop(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    songsLoaded = false;
    knLoginError = false;
}

void SongShop::updateCache()
{
    qWarning() << "Requesting songs from db.openkj.org";
    QJsonObject mainObject;
    mainObject.insert("command","getsongs");
    QJsonDocument jsonDocument;
    jsonDocument.setObject(mainObject);
    QNetworkRequest request(QUrl("https://db.openkj.org/apigetsongs"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->post(request, jsonDocument.toJson());
}

ShopSongs SongShop::getSongs()
{
    if (!songsLoaded)
        updateCache();
    return songs;
}

void SongShop::knLogin(QString userName, QString password)
{
    knLoginError = false;
    QByteArray md5hash = QCryptographicHash::hash(QByteArray::fromRawData((const char*)password.toLocal8Bit(), password.length()), QCryptographicHash::Md5).toHex();
    QString passHash = QString(md5hash);
    QString urlstr = "https://www.karaoke.net/songshop/cat/api_account_setup.php?action=validate_login&username=" + userName + "&md5=" + passHash + "&merchant=99";
    QUrl url = QUrl(urlstr);
    QNetworkRequest request(url);
    manager->get(request);
}

void SongShop::knPurchase(QString songId, QString ccNumber, QString ccM, QString ccY, QString ccCVV)
{
    QString urlstr = "https://www.karaoke.net/songshop/cat/api_make_order.php?";
    urlstr += "media_format=mp3g&";
    urlstr += "tracks=" + songId + "&";
    urlstr += "cc_number=" + ccNumber + "&";
    urlstr += "cc_cvv=" + ccCVV + "&";
    urlstr += "cc_exp_mm=" + ccM + "&";
    urlstr += "cc_exp_yyyy=" + ccY + "&";
    urlstr += "session_id=" + knSessionId + "&";
    urlstr += "merchant=99";
    QUrl url = QUrl(urlstr);
    QNetworkRequest request(url);
    manager->get(request);
}

bool SongShop::loggedIn()
{
    if (knSessionId != "")
        return true;
    return false;
}

void SongShop::setDlSongInfo(QString artist, QString title, QString songId)
{
    dlArtist = artist;
    dlTitle = title;
    dlSongId = songId;
}

void SongShop::downloadFile(const QString &url, const QString &destFn)
{
    QString destDir = settings->storeDownloadDir();
    if (!QDir(destDir).exists())
        QDir().mkdir(destDir);
    QString destPath = destDir + destFn;
    QNetworkAccessManager m_NetworkMngr;
    QNetworkReply *reply= m_NetworkMngr.get(QNetworkRequest(url));
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()),&loop, SLOT(quit()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    loop.exec();
    QUrl aUrl(url);
    QFileInfo fileInfo=aUrl.path();
    QFile file(destPath);
    file.open(QIODevice::WriteOnly);
    file.write(reply->readAll());
    delete reply;
    emit karaokeSongDownloaded(destPath);
}

void SongShop::onSslErrors(QNetworkReply *reply, QList<QSslError> errors)
{
    reply->abort();
    qWarning() << "Got ssl error";
    qWarning() << errors;
}

void SongShop::onNetworkReply(QNetworkReply *reply)
{
    qWarning() << "Got network reply from db.openkj.org";
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << reply->errorString();
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
    if (command == "getsongs")
    {
        emit songUpdateStarted();
        QJsonArray songsArray = json.object().value("songs").toArray();
        songs.clear();
        for (int i=0; i < songsArray.size(); i++)
        {
            ShopSong song;
            QJsonObject jsonObject = songsArray.at(i).toObject();
            song.artist = jsonObject.value("artist").toString();
            song.title = jsonObject.value("title").toString();
            song.songid = jsonObject.value("songid").toString();
            song.vendor = jsonObject.value("vendor").toString();
            song.price  = jsonObject.value("price").toDouble();
            song.type = 0;
            songs.append(song);
        }
        if (!songs.isEmpty())
            songsLoaded = true;
        emit songsUpdated();
    }
    else if ((json.object().value("result").toString() == "SUCCESS") && (json.object().value("session_id").toString() != ""))
    {
        knSessionId = json.object().value("session_id").toString();
        knLoginError = false;
        emit knLoginSuccess();
    }
    else if ((json.object().value("result").toString() == "ERROR") && (json.object().value("error").toString() == "Wrong username or password"))
    {
        qWarning() << "Login failed to Karoake.NET";
        qWarning() << "Incorrect username or password";
        knLoginError = true;
        knSessionId = "";
        emit knLoginFailure();
    }
    else if ((json.object().value("result").toString() == "SUCCESS") && (json.object().value("download_links").isArray()))
    {
        qWarning() << "Sucessfully purchased track";
        QString url = json.object().value("download_links").toArray().at(0).toString();
        qWarning()  <<  json.object().value("download_links").toArray();
        qWarning() << "Downloading";
        downloadFile(url, QString(dlSongId + " - " + dlArtist + " - " + dlTitle + ".zip"));
        qWarning() << "Done";

    }
    else if ((json.object().value("result").toString() == "ERROR") && (json.object().value("error").toString() == "Payment failed. Check your credit card details."))
    {
        emit paymentProcessingFailed();
    }
    else
        qWarning() << "JSON REPLY: " << data;


}

void SongShop::onDownloadProgress(qint64 received, qint64 total)
{
    qWarning() << "onDownloadProgress(" << received << "," << total << ") called";
    emit downloadProgress(received, total);
}
