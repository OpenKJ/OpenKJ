#include "dlgchat.h"
#include "ui_dlgchat.h"
#include <QScrollBar>

DlgChat::DlgChat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DlgChat)
{
    ui->setupUi(this);
    client = new MessagingClient(this);
    connect(client, &MessagingClient::messageReceived, this, &DlgChat::messageReceived);
    connect(client, &MessagingClient::singerListChanged, this, &DlgChat::singerListChanged);
    connect(client, &MessagingClient::historyReceived, this, &DlgChat::messageHistoryReceived);
    connect(client, &MessagingClient::disconnected, this, &DlgChat::disconnected);
    connect(ui->cmWidget, &ChatMessagesWidget::messagesUpdated, [this] () {
        ui->scrollArea_2->verticalScrollBar()->setSliderPosition(ui->scrollArea_2->verticalScrollBar()->maximum());
            });

    venueId = "isaactest";
    connectChat();
    QFont::insertSubstitution(QApplication::font().family(), "Noto Color Emoji");
}

DlgChat::~DlgChat()
{
    delete ui;
}

void DlgChat::setVenueId(const QString &venueId)
{
    this->venueId = venueId;

    client->venueId = venueId;
    client->signon();
}

void DlgChat::connectChat()
{
    client->fromUuid = venueId;
    client->toUuid = "";
    client->displayName = "KJ";
    client->apiKey = "someapikeydatahere";
    client->isSinger = false;
    client->venueId = venueId;
    QUrl url;
    url.setHost("messaging.openkj.org");
    url.setPort(8080);
    url.setScheme("ws");
    client->connectSocket(url);
}

void DlgChat::messageReceived(Message msg)
{
    ChatMessagesWidget::position pos;
    if (msg.fromSinger)
        pos = ChatMessagesWidget::LEFT;
    else
        pos = ChatMessagesWidget::RIGHT;
    ui->cmWidget->addMessage(msg.message, pos);
}

void DlgChat::messageHistoryReceived(QList<Message> messages)
{
    ui->cmWidget->clear();
    foreach (Message msg, messages)
    {
        messageReceived(msg);
    }
}

void DlgChat::disconnected()
{
    ui->cmWidget->clear();
    ui->listWidgetSingers->clear();
}

void DlgChat::singerListChanged(QList<IMSinger> singers)
{
    ui->listWidgetSingers->clear();
    for (int i=0; i < singers.size(); i++)
    {
        QListWidgetItem *it = new QListWidgetItem(singers.at(i).displayName);
        it->setData(Qt::UserRole, singers.at(i).uuid);
        ui->listWidgetSingers->addItem(it);
    }
}

void DlgChat::on_pushButtonSend_clicked()
{
    if (ui->listWidgetSingers->selectedItems().count() == 0)
    {
        qDebug() << "DlgChat - Send sent with no singer selected";
        return;
    }
    ui->cmWidget->addMessage(ui->lineEditMessage->text(), ChatMessagesWidget::RIGHT);
    client->sendIM(ui->lineEditMessage->text());
    ui->lineEditMessage->clear();
}

void DlgChat::on_listWidgetSingers_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current)
    {
        client->toUuid = QString();
        ui->cmWidget->clear();
        return;
    }
    client->toUuid = current->data(Qt::UserRole).toString();
    client->getMessageHistory(current->data(Qt::UserRole).toString());
}

void DlgChat::on_lineEditMessage_returnPressed()
{
    on_pushButtonSend_clicked();
}
