#include "chatmessageswidget.h"
#include <QTimer>
#include <QDebug>
#include <settings.h>

extern Settings *settings;

ChatMessagesWidget::ChatMessagesWidget(QWidget *parent) : QWidget(parent)
{

    vLayout = new QVBoxLayout(this);
    this->setLayout(vLayout);
    msgPalette.setColor(QPalette::Window,QColor(53,53,53));
//    msgPalette.setColor(QPalette::WindowText,Qt::white);
    msgPalette.setColor(QPalette::Base,QColor(42,42,42));
    msgPalette.setColor(QPalette::Window,QColor(53,53,53));
    msgPalette.setColor(QPalette::WindowText,Qt::white);
    msgPalette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
    msgPalette.setColor(QPalette::Base,QColor(42,42,42));
    msgPalette.setColor(QPalette::AlternateBase,QColor(66,66,66));
    msgPalette.setColor(QPalette::ToolTipBase,Qt::white);
    msgPalette.setColor(QPalette::ToolTipText,QColor(53,53,53));
//    msgPalette.setColor(QPalette::Text,Qt::white);
    msgPalette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
    msgPalette.setColor(QPalette::Dark,QColor(35,35,35));
    msgPalette.setColor(QPalette::Shadow,QColor(20,20,20));
    msgPalette.setColor(QPalette::Button,QColor(53,53,53));
    msgPalette.setColor(QPalette::ButtonText,Qt::white);
    msgPalette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
    msgPalette.setColor(QPalette::BrightText,Qt::red);
    msgPalette.setColor(QPalette::Link,QColor(42,130,218));
    msgPalette.setColor(QPalette::Highlight,QColor(42,130,218));
    msgPalette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
    msgPalette.setColor(QPalette::HighlightedText,Qt::white);
    msgPalette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));
    //this->setPalette(msgPalette);
    indentWidth = 30;
    QWidget *grower = new QWidget(this);
    QHBoxLayout *hLayout = new QHBoxLayout(grower);
    QSpacerItem *spacer = new QSpacerItem(0,5,QSizePolicy::Minimum, QSizePolicy::Expanding);
    grower->setLayout(hLayout);
    hLayout->addSpacerItem(spacer);
    vLayout->addWidget(grower);

}

void ChatMessagesWidget::clearLayout(QLayout *layout)
{
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
    clearLayout(vLayout);
    QWidget *grower = new QWidget(this);
    QHBoxLayout *hLayout = new QHBoxLayout(grower);
    QSpacerItem *spacer = new QSpacerItem(0,5,QSizePolicy::Minimum, QSizePolicy::Expanding);
    grower->setLayout(hLayout);
    hLayout->addSpacerItem(spacer);
    vLayout->addWidget(grower);
}

void ChatMessagesWidget::addMessage(QString message, int pos)
{
    QFont::insertSubstitution(QFont("Noto Sans Display").family(), "Noto Color Emoji");
    QFont::insertSubstitution(settings->applicationFont().family(), "Noto Color Emoji");
//    this->setFont(QFont("Noto Sans Display"));
    qWarning() << "Substitutes: " << QFont::substitutes(settings->applicationFont().family());

    QWidget *msgWidget = new QWidget(this);
    msgWidget->setContentsMargins(0,0,0,0);
    QHBoxLayout *hLayout = new QHBoxLayout(msgWidget);
    msgWidget->setLayout(hLayout);
    QSpacerItem *spacer = new QSpacerItem(indentWidth,0,QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    QLabel *label = new QLabel(message, this);
    QFont imFont = settings->applicationFont();
    label->setFont(imFont);

    label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    auto lpalette = label->palette();
    lpalette.setColor(QPalette::WindowText, Qt::white);
    lpalette.setColor(QPalette::Window, Qt::black);
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
    vLayout->addWidget(msgWidget);
    QTimer timer;
    QTimer::singleShot(150, [this] () { emit messagesUpdated(); });


}

