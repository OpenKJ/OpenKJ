#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H

#include <QJsonDocument>
#include <QObject>
#include <QJsonObject>
#include <QWebSocket>
#include <QJsonArray>
#include <QTimer>
#include "messagingtypes.h"

struct IMSinger {
      QString displayName;
      QString uuid;
};


class MessagingClient : public QObject
{
    Q_OBJECT
    QTimer timer;
    QUrl url;
public:
    explicit MessagingClient(QObject *parent);
    QWebSocket m_webSocket;
    QUrl m_url;
    void signon();
    QString fromUuid;
    QString toUuid;
    QString displayName;
    QString venueId;
    QString apiKey;
    bool isSinger;
    bool reconnect;
    QList<IMSinger> currentSingerList;

signals:
  //  void messageReceived(const QString&);
    void disconnected();
    void connected();
    void singerListChanged(QList<IMSinger>);
    void messageReceived(Message);
    void historyReceived(QList<Message>);

public slots:
    void connectSocket(QUrl url);
    void disconnect();
    void sendMessage(const QString& message);
    void changeDisplayName(const QString& newName);
    void sendIM(QString message);
    void getMessageHistory(const QString &uuid);
    void setVenue(const QString &venueId);


private slots:
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &data);
    void updateSingers(const QByteArray &data);
    void updateHistory(const QByteArray &data);
    void onConnected();
    void closed();
    void onError(QAbstractSocket::SocketError error);
    void stateChanged(QAbstractSocket::SocketState state);

};

#endif // ECHOCLIENT_H
