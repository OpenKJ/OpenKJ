#ifndef OKJSONGBOOKAPI_H
#define OKJSONGBOOKAPI_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QDebug>


class OkjsRequest
{
public:
    int requestId;
    QString singer;
    QString artist;
    QString title;
    int time;
};


typedef QList<OkjsRequest> OkjsRequests;

class OkjsVenue
{
public:
    int venueId;
    QString name;
    QString urlName;
    bool accepting;
    bool operator == (const OkjsVenue& v) const;
};

QDebug operator<<(QDebug dbg, const OkjsVenue &okjsvenue);

typedef QList<OkjsVenue> OkjsVenues;

class OKJSongbookAPI : public QObject
{
    Q_OBJECT
private:
    QString apiKey;
    int serial;
    QUrl serverUrl;
    OkjsVenues venues;
    QNetworkAccessManager *manager;

public:
    explicit OKJSongbookAPI(QObject *parent = 0);
    int getSerial();
    OkjsRequests refreshRequests();
    void removeRequest(int requestId);
    bool requestsEnabled();
    void setRequestsEnabled(bool enabled);
    void setApiKey(QString apiKey);
    OkjsVenues refreshVenues();
    OkjsVenues getVenues();
    void clearRequests();

signals:
    void serialChanged();
    void venuesChanged();
    void sslError();

public slots:

private slots:
        void onSslErrors(QNetworkReply * reply, QList<QSslError> errors);
};

#endif // OKJSONGBOOKAPI_H
