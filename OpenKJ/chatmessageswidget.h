#ifndef CHATMESSAGESWIDGET_H
#define CHATMESSAGESWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QLabel>
#include <QTimer>

class ChatMessagesWidget : public QWidget
{
    Q_OBJECT
    QVBoxLayout *vLayout;
    int indentWidth;
    void clearLayout(QLayout *layout);
    QPalette msgPalette;
    QTimer timer;

public:
    enum position {LEFT, RIGHT};
    explicit ChatMessagesWidget(QWidget *parent = nullptr);
    void clear();
    void addMessage(QString message, int pos);
signals:
    void messagesUpdated();
};

#endif // CHATMESSAGESWIDGET_H
