#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include "../SongbookIMServer/messagingtypes.h"


class MessagesModel : public QAbstractListModel
{
    Q_OBJECT
private:
    QList<Message> messages;
public:
    MessagesModel();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    void appendMessage(const Message& message);

};

#endif // MESSAGESMODEL_H
