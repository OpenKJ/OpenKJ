#ifndef MESSAGINGTYPES_H
#define MESSAGINGTYPES_H

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QWebSocket>
#include <QJsonArray>


enum MsgType {
    REGISTRATION,
    IM,
    DISPLAY_NAME_CHANGE,
    VENUE_CHANGE,
    VENUE_SINGERS,
    CLIENT_CONNECT_ACK,
    GET_SINGERS,
    GET_MSG_HISTORY,
    MSG_HISTORY,
    MARK_CONV_READ,
    MARK_MSG_RECEIVED,
    MARK_CONV_RECEIVED
};

class DisplayNameChange {
public:
    int dataType;
    QString uuid;
    QString newName;
    DisplayNameChange() {
        dataType = MsgType::DISPLAY_NAME_CHANGE;
    }
    DisplayNameChange(const QByteArray &json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::DISPLAY_NAME_CHANGE;
        uuid = doc.object().value("uuid").toString();
        newName = doc.object().value("newName").toString();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::DISPLAY_NAME_CHANGE);
        jsonObject.insert("uuid", uuid);
        jsonObject.insert("newName", newName);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class VenueChange {
public:
    int dataType;
    QString uuid;
    QString newVenueId;
    VenueChange() {
        dataType = MsgType::VENUE_CHANGE;
    }
    VenueChange(const QByteArray &json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::VENUE_CHANGE;
        uuid = doc.object().value("uuid").toString();
        newVenueId = doc.object().value("newVenue").toString();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::VENUE_CHANGE);
        jsonObject.insert("uuid", uuid);
        jsonObject.insert("newVenue", newVenueId);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class GetMsgHistory {
public:
    int dataType;
    QString uuid1;
    QString uuid2;
    GetMsgHistory() {
        dataType = MsgType::GET_MSG_HISTORY;
    }
    GetMsgHistory(const QByteArray& json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::VENUE_CHANGE;
        uuid1 = doc.object().value("uuid1").toString();
        uuid2 = doc.object().value("uuid2").toString();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::GET_MSG_HISTORY);
        jsonObject.insert("uuid1", uuid1);
        jsonObject.insert("uuid2", uuid2);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class MarkConvRead {
public:
    int dataType;
    QString toUuid;
    QString fromUuid;
    MarkConvRead() {
        dataType = MARK_CONV_READ;
    }
    MarkConvRead(const QByteArray& json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::MARK_CONV_READ;
        toUuid = doc.object().value("toUuid").toString();
        fromUuid = doc.object().value("fromUuid").toString();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::MARK_CONV_READ);
        jsonObject.insert("toUuid", toUuid);
        jsonObject.insert("fromUuid", fromUuid);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class MarkConvReceived {
public:
    int dataType;
    QString toUuid;
    QString fromUuid;
    MarkConvReceived() {
        dataType = MARK_CONV_RECEIVED;
    }
    MarkConvReceived(const QByteArray& json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::MARK_CONV_RECEIVED;
        toUuid = doc.object().value("toUuid").toString();
        fromUuid = doc.object().value("fromUuid").toString();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::MARK_CONV_RECEIVED);
        jsonObject.insert("toUuid", toUuid);
        jsonObject.insert("fromUuid", fromUuid);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class MarkMsgReceived {
public:
    int dataType;
    int msgId;
    QString toUuid;
    QString fromUuid;
    MarkMsgReceived() {
        dataType = MARK_MSG_RECEIVED;
    }
    MarkMsgReceived(const QByteArray& json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::MARK_MSG_RECEIVED;
        toUuid = doc.object().value("toUuid").toString();
        fromUuid = doc.object().value("fromUuid").toString();
        msgId = doc.object().value("msgId").toInt();
    }
    QByteArray toJson() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::MARK_MSG_RECEIVED);
        jsonObject.insert("toUuid", toUuid);
        jsonObject.insert("fromUuid", fromUuid);
        jsonObject.insert("msgId", msgId);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class Message {
public:
    qint64 msgId;
    bool fromSinger;
    int dataType;
    QString fromUuid;
    QString toUuid;
    QString venueId;
    QString senderDisplayName;
    QString message;
    QDateTime createdTime;
    QDateTime receiptTime;
    QDateTime readTime;
    Message() {
        createdTime = QDateTime::currentDateTimeUtc();
        dataType = MsgType::IM;
    }
    Message(const QByteArray &json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        msgId = doc.object().value("msgId").toInt();
        dataType = MsgType::IM;
        fromSinger = doc.object().value("fromSinger").toBool();
        fromUuid = doc.object().value("fromUuid").toString();
        toUuid = doc.object().value("toUuid").toString();
        venueId = doc.object().value("venueId").toString();
        senderDisplayName = doc.object().value("senderDisplayName").toString();
        message = doc.object().value("message").toString();
        createdTime = QDateTime::fromTime_t(doc.object().value("createdTime").toInt());
        receiptTime = QDateTime::fromTime_t(doc.object().value("receiptTime").toInt());
        readTime = QDateTime::fromTime_t(doc.object().value("readTime").toInt());
    }
    QByteArray toJson() const {
        return QJsonDocument(toJsonObject()).toJson(QJsonDocument::Compact);
    }
    QJsonObject toJsonObject() const {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::IM);
        jsonObject.insert("msgId", QJsonValue(msgId));
        jsonObject.insert("fromSinger", fromSinger);
        jsonObject.insert("fromUuid", fromUuid);
        jsonObject.insert("toUuid", toUuid);
        jsonObject.insert("venueId", venueId);
        jsonObject.insert("senderDisplayName", senderDisplayName);
        jsonObject.insert("message", message);
        jsonObject.insert("createdTime", createdTime.toMSecsSinceEpoch());
        jsonObject.insert("receiptTime", receiptTime.toMSecsSinceEpoch());
        jsonObject.insert("readTime", readTime.toMSecsSinceEpoch());
        return jsonObject;
    }
};

class MsgHistory {
public:
    int dataType;
    QString uuid1;
    QString uuid2;
    std::vector<Message> messages;
    MsgHistory() {
        dataType = MsgType::MSG_HISTORY;
    }
    MsgHistory(const QByteArray& json) {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::MSG_HISTORY;
        uuid1 = doc.object().value("uuid1").toString();
        uuid2 = doc.object().value("uuid2").toString();
        QJsonArray arr = doc.object().value("messages").toArray();
        std::for_each(arr.begin(), arr.end(), [&] (auto msg) {
           messages.push_back(QJsonDocument(msg.toObject()).toJson());
        });
    }
    QByteArray toJson() const {
        QJsonArray msgArray;
        std::for_each(messages.begin(), messages.end(), [&] (auto msg) {
           msgArray.append(msg.toJsonObject());
        });
        QJsonObject jsonObject;
        jsonObject.insert("dataType", MsgType::MSG_HISTORY);
        jsonObject.insert("uuid1", uuid1);
        jsonObject.insert("uuid2", uuid2);
        jsonObject.insert("messages", msgArray);
        return QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
};

class Peer {
public:
    QString uuid;
    bool isSinger;
    QWebSocket *socket;
    QString displayName;
    QString venueId;
    Peer() {
        isSinger = false;
    }
};


class Venue {
public:
    QString venueId;
    std::vector<Message> messages;
    bool online;
    Venue() {
        online = false;
    }
};


class Registration {
public:
    int dataType;
    QString displayName;
    QString uuid;
    QString apiKey;
    QString venueId;
    bool isSinger;
    bool success;
    Registration() {
        dataType = MsgType::REGISTRATION;
        isSinger = false;
        success = false;
    }
    Registration(QByteArray json)
    {
        QJsonDocument doc = QJsonDocument::fromJson(json);
        dataType = MsgType::REGISTRATION;
        displayName = doc.object().value("displayName").toString();
        uuid = doc.object().value("uuid").toString();
        apiKey = doc.object().value("apiKey").toString();
        isSinger = doc.object().value("isSinger").toBool();
        success = doc.object().value("success").toBool();
        venueId = doc.object().value("venueId").toString();
    }
    QByteArray toJson() {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", dataType);
        jsonObject.insert("displayName", displayName);
        jsonObject.insert("isSinger", isSinger);
        jsonObject.insert("uuid", uuid);
        jsonObject.insert("success", success);
        jsonObject.insert("apiKey", apiKey);
        jsonObject.insert("venueId", venueId);
        return QJsonDocument(jsonObject).toJson();
    }
};

class RegisteredSingers {
public:
    QString venueId;
    std::vector<Peer> singers;
    QByteArray getJson() {
        QJsonObject obj;
        obj.insert("dataType", MsgType::VENUE_SINGERS);
        QJsonArray arr;
        std::for_each(singers.begin(), singers.end(), [&] (auto singer) {
           QJsonObject singerObj;
           singerObj.insert("displayName", singer.displayName);
           singerObj.insert("venueId", singer.venueId);
           singerObj.insert("uuid", singer.uuid);
           arr.append(singerObj);
        });
        obj.insert("singers", arr);
        return QJsonDocument(obj).toJson();
    }
};

class ClientConnectAck {
public:
    int dataType;
    ClientConnectAck() {
        dataType = MsgType::CLIENT_CONNECT_ACK;
    }
    QByteArray toJson() {
        QJsonObject jsonObject;
        jsonObject.insert("dataType", dataType);
        return QJsonDocument(jsonObject).toJson();
    }
};




#endif // MESSAGINGTYPES_H
