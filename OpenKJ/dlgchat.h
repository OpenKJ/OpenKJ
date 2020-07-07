#ifndef DLGCHAT_H
#define DLGCHAT_H

#include <QListWidget>
#include <QWidget>
#include <vector>
#include "messagingtypes.h"
#include "messagingclient.h"

namespace Ui {
class DlgChat;
}

class DlgChat : public QWidget
{
    Q_OBJECT
private:
    MessagingClient client;
    Ui::DlgChat *ui;
    QString venueId;

public:
    explicit DlgChat(QWidget *parent = nullptr);
    ~DlgChat();

public slots:
    void setVenueId(const QString &venueId);
    void connectChat();

private slots:
    void messageReceived(const Message &msg);
    void messageHistoryReceived(const std::vector<Message> &messages);
    void disconnected();
    void singerListChanged(const std::vector<IMSinger> &singers);
    void on_pushButtonSend_clicked();
    void on_listWidgetSingers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_lineEditMessage_returnPressed();
};

#endif // DLGCHAT_H
