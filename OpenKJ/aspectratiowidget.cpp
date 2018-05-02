#include "aspectratiowidget.h"

AspectRatioWidget::AspectRatioWidget(QWidget *parent) :
    QWidget(parent)
{
    layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->setMargin(0);

    vidWidget = new CdgVideoWidget(this);
    arWidth = 16;
    arHeight = 9;
    // add spacer, then your widget, then spacer
    layout->addItem(new QSpacerItem(0, 0));
    layout->addWidget(vidWidget);
    layout->addItem(new QSpacerItem(0, 0));
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, QColor("yellow"));
    palette.setColor(QPalette::Window, QColor("yellow"));
    this->setPalette(palette);
}

void AspectRatioWidget::resizeEvent(QResizeEvent *event)
{
    float thisAspectRatio = (float)event->size().width() / event->size().height();
    int widgetStretch, outerStretch;

    if (thisAspectRatio > (arWidth/arHeight)) // too wide
    {
        layout->setDirection(QBoxLayout::LeftToRight);
        widgetStretch = height() * (arWidth/arHeight); // i.e., my width
        outerStretch = (width() - widgetStretch) / 2 + 0.5;
    }
    else // too tall
    {
        layout->setDirection(QBoxLayout::TopToBottom);
        widgetStretch = width() * (arHeight/arWidth); // i.e., my height
        outerStretch = (height() - widgetStretch) / 2 + 0.5;
    }

    layout->setStretch(0, outerStretch);
    layout->setStretch(1, widgetStretch);
    layout->setStretch(2, outerStretch);
}
