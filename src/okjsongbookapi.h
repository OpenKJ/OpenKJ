#ifndef OKJSONGBOOKAPI_H
#define OKJSONGBOOKAPI_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include "settings.h"


class OkjsRequest
{
public:
    int requestId;
    QString singer;
    QString artist;
    QString title;
    int key;
    int time;
    bool operator == (const OkjsRequest& r) const;
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
    int serial;
    OkjsVenues venues;
    OkjsRequests requests;
    QNetworkAccessManager *manager;
    QTimer *timer;
    QTimer *alertTimer;
    QTime lastSync;
    bool delayErrorEmitted;
    bool connectionReset;
    int entitledSystems;
    bool programIsIdle;
    bool cancelUpdate;
    bool updateInProgress;
    Settings m_settings;

public:
    explicit OKJSongbookAPI(QObject *parent = nullptr);
    void getSerial();
    void refreshRequests();
    void removeRequest(int requestId);
    bool getAccepting();
    void setAccepting(bool enabled);
    void refreshVenues(bool blocking = false);
    void clearRequests();
    void updateSongDb();
    bool test();
    void alertCheck();
    void getEntitledSystemCount();
    [[nodiscard]] int entitledSystemCount() const { return entitledSystems; }
    [[nodiscard]] bool updateWasCancelled() const {return cancelUpdate; }
    void triggerTestAdd();

signals:
    void venuesChanged(OkjsVenues);
    void sslError();
    void remoteSongDbUpdateProgress(int);
    void remoteSongDbUpdateNumDocs(int);
    void remoteSongDbUpdateDone();
    void remoteSongDbUpdateStart();
    void requestsChanged(OkjsRequests);
    void synchronized(QTime);
    void delayError(int);
    void testPassed();
    void testFailed(QString error);
    void testSslError(QString error);
    void alertRecieved(QString title, QString message);
    void entitledSystemCountChanged(int count);


public slots:
    void dbUpdateCanceled();
    void setInterval(int interval);


private slots:
        void onSslErrors(QNetworkReply * reply, const QList<QSslError>& errors);
        void onTestSslErrors(QNetworkReply * reply, QList<QSslError> errors);
        void onNetworkReply(QNetworkReply* reply);
        void timerTimeout();
        void alertTimerTimeout();
        void idleStateChanged(bool isIdle);
};

#endif // OKJSONGBOOKAPI_H
