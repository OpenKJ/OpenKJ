#ifndef SONGSHOP_H
#define SONGSHOP_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <QDebug>
#include "settings.h"


class ShopSong
{
public:
    operator QString() const { return QString("Artist: " + artist + " Title: " + title + " SongId: " + songid + " Vendor: " + vendor + " Price: " + QString("%1").arg(price)); }
    QString artist;
    QString title;
    QString songid;
    QString vendor;
    int type;
    double price;
    bool operator == (const ShopSong r) const;
};


typedef QList<ShopSong> ShopSongs;



class SongShop : public QObject
{
    Q_OBJECT
public:
    explicit SongShop(QObject *parent = 0);
    void updateCache();
    ShopSongs getSongs();
    void knLogin(QString userName, QString password);
    void knPurchase(QString songId, QString ccNumber, QString ccM, QString ccY, QString ccCVV);
    bool loggedIn();
    bool loginError() { return knLoginError; }
    void setDlSongInfo(QString artist, QString title, QString songId);

private:
    QNetworkAccessManager *manager;
    bool connectionReset;
    bool songsLoaded;
    ShopSongs songs;
    QString knSessionId;
    bool knLoginError;
    void downloadFile(const QString &url, const QString &destFn);
    QString dlArtist;
    QString dlTitle;
    QString dlSongId;
    Settings m_settings;

signals:
    void songUpdateStarted();
    void songsUpdated();
    void knLoginSuccess();
    void knLoginFailure();
    void karaokeSongDownloaded(QString path);
    void paymentProcessingFailed();
    void downloadProgress(qint64 received, qint64 total);

public slots:

private slots:
    void onSslErrors(QNetworkReply * reply, QList<QSslError> errors);
    void onNetworkReply(QNetworkReply* reply);
    void onDownloadProgress(qint64 received, qint64 total);
};

#endif // SONGSHOP_H
