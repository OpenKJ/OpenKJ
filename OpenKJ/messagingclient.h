#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H

#include <QJsonDocument>
#include <QObject>
#include <QJsonObject>
#include <QWebSocket>
#include <QJsonArray>
#include <QTimer>
#include <vector>
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
    MessagingClient();
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
    std::vector<IMSinger> currentSingerList;

signals:
  //  void messageReceived(const QString&);
    void disconnected();
    void connected();
    void singerListChanged(const std::vector<IMSinger>&);
    void messageReceived(const Message&);
    void historyReceived(const std::vector<Message>&);

public slots:
    void connectSocket(QUrl url);
    void disconnect();
    void sendMessage(const QString& message);
    void changeDisplayName(const QString& newName);
    void sendIM(QString message);
    void getMessageHistory(const QString &uuid);
    void setVenue(const QString &venueId);
    void markConvRead(const QString &fromUuid);
    void markConvReceived(const QString &fromUuid);
    void markMsgRecieved(const QString &fromUuid, const int msgId);


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
