#include "messagesmodel.h"
#include <QColor>
#include <QBrush>

MessagesModel::MessagesModel()
{

}

int MessagesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return messages.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const
{
    QString msg;
    if (role == Qt::DisplayRole)
    {
    if (messages.at(index.row()).fromSinger)
        msg = messages.at(index.row()).message;
    else
        msg = messages.at(index.row()).message;
    return msg;
    }
    else if (role == Qt::TextAlignmentRole)
    {
            if (messages.at(index.row()).fromSinger)
                return Qt::AlignLeft;
            else
                return Qt::AlignRight;
    }
    else if (role == Qt::BackgroundRole)
    {
            if (messages.at(index.row()).fromSinger)
                return QBrush(QColor::fromRgb(100,100,255));
            else
                return QBrush(QColor::fromRgb(100,100,175));
    }
    else if (role == Qt::ForegroundRole)
    {
            if (messages.at(index.row()).fromSinger)
                return QBrush(QColor::fromRgb(255,255,255));
            else
                return QBrush(QColor::fromRgb(255,255,255));
    }

    return QVariant();
}

void MessagesModel::appendMessage(const Message &message)
{
    emit layoutAboutToBeChanged();
    messages.push_back(message);
    emit layoutChanged();
}
