#ifndef CHATMESSAGESWIDGET_H
#define CHATMESSAGESWIDGET_H

#include <QWidget>
#include <QVBoxLayout>

class ChatMessagesWidget : public QWidget
{
    Q_OBJECT
private:
    QVBoxLayout *m_vLayout;
    int m_indentWidth;
    void clearLayout(QLayout *layout);

public:
    enum position {LEFT, RIGHT};
    explicit ChatMessagesWidget(QWidget *parent = nullptr);
    void clear();
    void addMessage(const QString &message, const int &pos);

signals:
    void messagesUpdated();
};

#endif // CHATMESSAGESWIDGET_H
