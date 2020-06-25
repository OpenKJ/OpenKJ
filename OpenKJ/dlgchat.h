#ifndef DLGCHAT_H
#define DLGCHAT_H

#include <QListWidget>
#include <QWidget>
#include "messagingtypes.h"
#include "messagingclient.h"

namespace Ui {
class DlgChat;
}

class DlgChat : public QWidget
{
    Q_OBJECT
private:
    MessagingClient *client;

public:
    explicit DlgChat(QWidget *parent = nullptr);
    ~DlgChat();

private:
    Ui::DlgChat *ui;
    QString venueId;

public slots:
    void setVenueId(const QString &venueId);
    void connectChat();

private slots:
    void messageReceived(Message msg);
    void messageHistoryReceived(QList<Message> messages);
    void disconnected();
    void singerListChanged(QList<IMSinger> singers);
    void on_pushButtonSend_clicked();
    void on_listWidgetSingers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_lineEditMessage_returnPressed();
};

#endif // DLGCHAT_H
