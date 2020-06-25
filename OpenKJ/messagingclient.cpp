#include "messagingclient.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>




MessagingClient::MessagingClient(QObject *parent) :
    QObject(parent)
{
    reconnect = false;
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MessagingClient::closed);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &MessagingClient::onError);
    connect(&m_webSocket, &QWebSocket::stateChanged, this, &MessagingClient::stateChanged);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &MessagingClient::onTextMessageReceived);
}

void MessagingClient::signon()
{
    Registration reg;
    reg.uuid = fromUuid;
    reg.isSinger = isSinger;
    reg.displayName = displayName;
    reg.venueId = venueId;
    m_webSocket.sendBinaryMessage(reg.toJson());
}

void MessagingClient::connectSocket(QUrl url)
{
    this->url = url;
    m_webSocket.open(url);
    connect(&timer, &QTimer::timeout, [this] () {
        m_webSocket.ping();
    });
    timer.start(5000);
    reconnect = true;
}

void MessagingClient::disconnect()
{
    reconnect = false;
    m_webSocket.close();
}

void MessagingClient::sendMessage(const QString &message)
{
    m_webSocket.sendTextMessage(message);
}

void MessagingClient::changeDisplayName(const QString &newName)
{
    DisplayNameChange chg;
    chg.newName = newName;
    chg.uuid = fromUuid;
    m_webSocket.sendBinaryMessage(chg.toJson());
}



void MessagingClient::onConnected()
{
    qInfo() << "MessagingClient - WebSocket connected";
    qInfo() << "MessagingClient - Sending registration message";
    signon();
}

void MessagingClient::closed()
{
    emit disconnected();
}

void MessagingClient::onError(QAbstractSocket::SocketError error)
{
    qInfo() << "MessagingClient - Socket error: " << error;
    timer.stop();
}

void MessagingClient::stateChanged(QAbstractSocket::SocketState state)
{
    qInfo() << "MessagingClient - Socket state change: " << state;
    switch (state)
    {
    case QAbstractSocket::ConnectedState:
        signon();
        break;
    case QAbstractSocket::ConnectingState:
        break;
    default:
        timer.stop();
        emit disconnect();
        if (reconnect)
            connectSocket(url);
    }
}

void MessagingClient::sendIM(QString message)
{
    Message msg;
    msg.fromUuid = this->fromUuid;
    msg.toUuid = this->toUuid;
    msg.message = message;
    msg.venueId = this->venueId;
    msg.fromSinger = this->isSinger;
    m_webSocket.sendBinaryMessage(msg.toJson());
}

void MessagingClient::getMessageHistory(const QString &uuid)
{
    GetMsgHistory gmh;
    gmh.uuid1 = this->venueId;
    gmh.uuid2 = uuid;
    m_webSocket.sendBinaryMessage(gmh.toJson());
}

void MessagingClient::setVenue(const QString &venueId)
{
    reconnect = false;
    disconnect();
    this->venueId = venueId;
    this->fromUuid = venueId;
    connectSocket(this->url);

}

void MessagingClient::onTextMessageReceived(const QString &message)
{
    onBinaryMessageReceived(message.toLocal8Bit());
}

void MessagingClient::onBinaryMessageReceived(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    int docType = doc.object().value("dataType").toInt();

    switch (docType) {
    case MsgType::IM:
        qInfo() << "MessagingClient - Received new IM";
        emit messageReceived(Message(data));
        break;
    case MsgType::REGISTRATION:
        qInfo() << "MessagingClient - Received registration message";
        break;
    case MsgType::VENUE_SINGERS:
        qInfo() << "MessagingClient - Received updated singers data";
        updateSingers(data);
        break;
    case MsgType::MSG_HISTORY:
        qInfo() << "MessagingClient - Received message history data";
        updateHistory(data);
        break;
    case MsgType::CLIENT_CONNECT_ACK:
        qInfo() << "MessagingClient - Received client connnection ack message";
        break;
    default:
        qInfo() << "MessagingClient - Unhandled data type - Message binary Contents:\n" << data;
        break;
    }

}


void MessagingClient::updateSingers(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    currentSingerList.clear();
    qInfo() << "MessagingClient - Received registered singers data";
    QJsonArray arr = doc.object().value("singers").toArray();
    for (int i=0; i < arr.size(); i++)
    {
        IMSinger singer;
        singer.displayName = arr.at(i).toObject().value("displayName").toString();
        singer.uuid = arr.at(i).toObject().value("uuid").toString();
        currentSingerList.push_back(singer);
    }
    emit singerListChanged(currentSingerList);
}

void MessagingClient::updateHistory(const QByteArray &data)
{
    MsgHistory mh(data);
    emit historyReceived(mh.messages);
}
