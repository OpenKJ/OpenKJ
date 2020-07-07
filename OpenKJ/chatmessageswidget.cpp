#include "chatmessageswidget.h"
#include <QTimer>
#include <QLabel>
#include <QSpacerItem>
#include <QHBoxLayout>
#include <QDebug>
#include <settings.h>

extern Settings *settings;

ChatMessagesWidget::ChatMessagesWidget(QWidget *parent) : QWidget(parent)
{
    m_vLayout = new QVBoxLayout(this);
    this->setLayout(m_vLayout);
    m_indentWidth = 30;
    auto grower = new QWidget(this);
    auto hLayout = new QHBoxLayout(grower);
    auto spacer = new QSpacerItem(0,5,QSizePolicy::Minimum, QSizePolicy::Expanding);
    grower->setLayout(hLayout);
    hLayout->addSpacerItem(spacer);
    m_vLayout->addWidget(grower);
}

void ChatMessagesWidget::clearLayout(QLayout *layout)
{
    // Removes all children from a layout since QLayout doesn't have a recursive clear function

    QLayoutItem* child;
    while (layout->count() != 0)
    {
        child = layout->takeAt(0);
        if (child->layout() != 0)
        {
            clearLayout(child->layout());
        }
        else if (child->widget() != 0)
        {
            delete child->widget();
        }
        delete child;
    }
}

void ChatMessagesWidget::clear()
{
    // Completely clears the message contents and reset back to an empty state

    clearLayout(m_vLayout);
    auto grower = new QWidget(this);
    auto hLayout = new QHBoxLayout(grower);
    auto spacer = new QSpacerItem(0,5,QSizePolicy::Minimum, QSizePolicy::Expanding);
    grower->setLayout(hLayout);
    hLayout->addSpacerItem(spacer);
    m_vLayout->addWidget(grower);
}

void ChatMessagesWidget::addMessage(const QString &message, const int &pos)
{
    // Adds a message to the chat messages widget

    // this enables color emoji substitution characters
    QFont::insertSubstitution(QFont("Noto Sans Display").family(), "Noto Color Emoji");
    QFont::insertSubstitution(settings->applicationFont().family(), "Noto Color Emoji");

    auto msgWidget = new QWidget(this);
    msgWidget->setContentsMargins(0,0,0,0);
    auto hLayout = new QHBoxLayout(msgWidget);
    msgWidget->setLayout(hLayout);
    auto spacer = new QSpacerItem(m_indentWidth,0,QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    auto label = new QLabel(message, this);
    auto imFont = settings->applicationFont();
    label->setFont(imFont);
    label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    auto lpalette = label->palette();
    lpalette.setColor(QPalette::WindowText, Qt::white);
    label->setPalette(lpalette);
    label->setAutoFillBackground(true);
    label->setMargin(15);
    label->setWordWrap(true);
    label->setMinimumHeight(50);
    if (pos == LEFT)
    {
        label->setStyleSheet("QLabel{margin-left: 10px; border-radius: 15px; background: #01579B; color: #FFFFFF;}");
        hLayout->addWidget(label);
        hLayout->addSpacerItem(spacer);
    }
    else
    {
        label->setStyleSheet("QLabel{margin-left: 10px; border-radius: 15px; background: #546E7A; color: #FFFFFF;}");
        hLayout->addSpacerItem(spacer);
        hLayout->addWidget(label);
    }
    m_vLayout->addWidget(msgWidget);

    // We wait to emit this signal to give the UI time to add the message widget, otherwise the receiver won't scroll
    // the view all the way to the bottom with the new message
    QTimer::singleShot(150, [this] () { emit messagesUpdated(); });
}

