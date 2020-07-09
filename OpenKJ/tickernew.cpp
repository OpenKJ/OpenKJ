#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>
#include <QOpenGLWidget>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <settings.h>
#include <QMutex>
//#include <QGLWidget>

QMutex mutex;


TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QGraphicsView(parent)
{
    underflow = false;
    heightHint = 100;
    jumpPoint = 0;
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHints(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheNone);
    speed = 25;
    spm = new QGraphicsPixmapItem(getPixmapFromString("Placeholder txt"));
    spm2 = new QGraphicsPixmapItem(getPixmapFromString("Placeholder Text"));
    scene->addItem(spm);
    spm->setPos(QPointF(0.0f, 0.0f));
    spm2->setPos(spm->pos().x() + spm->boundingRect().right(), spm->pos().y());
    timer.setInterval(3);
    speed = settings.tickerSpeed();
    pixelShift = (float)this->speed / 10.0f;
    QObject::connect(&timer, &QTimer::timeout, [this]() {
        if (underflow)
        {
            if (spm->pos() != QPoint(0,0))
                spm->setPos(0.0f,0.0f);
            if (spm2->isVisible())
                spm2->hide();
            return;
        }
        else
        {
            if (!spm2->isVisible())
                spm2->show();
        }
        if (!flipped)
        {
            qreal xpos = spm->pos().x();
            spm->setPos(QPointF(xpos - pixelShift, 0.0f));
            spm2->setPos(spm->pos().x() + spm->boundingRect().right(), spm->pos().y());
            if (xpos + spm->boundingRect().right() < 0)
            {
                spm->setPos(spm2->pos().x() + spm2->boundingRect().right(), spm2->pos().y());
                flipped = true;
            }
        }
        else
        {
            qreal xpos = spm2->pos().x();
            spm2->setPos(QPointF(xpos - pixelShift, 0.0f));
            spm->setPos(spm2->pos().x() + spm2->boundingRect().right(), spm2->pos().y());
            if (xpos + spm2->boundingRect().right() < 0)
            {
                spm2->setPos(spm->pos().x() + spm->boundingRect().right(), spm->pos().y());
                flipped = false;
            }
        }
    });
    timer.start();
}

TickerDisplayWidget::~TickerDisplayWidget()
{

}

void TickerDisplayWidget::setText(const QString &newText)
{
    currentTxt = newText;
    timer.stop();
    scene->removeItem(spm);
    scene->removeItem(spm2);
    delete spm;
    delete spm2;
    spm = new QGraphicsPixmapItem(getPixmapFromString(newText));
    spm2 = new QGraphicsPixmapItem(getPixmapFromString(newText));
    scene->addItem(spm);
    scene->addItem(spm2);
    flipped = false;
    spm->setPos(QPointF(rect().left(), 0.0f));
    spm2->setPos(spm->pos().x() + spm->boundingRect().right(), spm->pos().y());

    timer.start();
}


void TickerDisplayWidget::setSpeed(int speed)
{

    if (speed > 50)
        this->speed = 50;
    else this->speed = speed;
    pixelShift = (float)speed / 10.0f;
}

void TickerDisplayWidget::stop()
{
    timer.stop();
}

void TickerDisplayWidget::setTickerEnabled(const bool& enabled)
{
    if (enabled)
        timer.start();
    else
        timer.stop();
}

void TickerDisplayWidget::refreshTickerSettings()
{
    setText(currentTxt);
    // setBackgroundBrush(QBrush(settings.tickerBgColor()));
    // setAutoFillBackground(true);
}

QPixmap TickerDisplayWidget::getPixmapFromString(const QString& text)
{
    auto tickerWidth = this->visibleRegion().boundingRect().width();
    auto tickerFont = settings.tickerFont();
    auto metrics = QFontMetrics(tickerFont);
    QString drawText;
    int pxWidth;
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    if (tickerWidth >= metrics.horizontalAdvance(text))
#else
    if (myWidth >= metrics.width(text))
#endif
    {
        pxWidth = tickerWidth * 2;
        underflow = true;
        drawText = text;
        jumpPoint = 99999;

    }
    else
    {
        drawText = " " + text + " |";
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
        pxWidth = metrics.horizontalAdvance(drawText);
#else
        pxWidth = metrics.width(drawText);
#endif
        underflow = false;
    }
    QPixmap img = QPixmap(pxWidth, metrics.boundingRect(drawText).height() + 30);
    img.fill(settings.tickerBgColor());
    QPainter p;
    p.begin(&img);
    p.setPen(QPen(settings.tickerTextColor()));
    p.setFont(tickerFont);
    p.drawText(img.rect() /*.adjusted(0,3,0,3)*/, Qt::AlignLeft | Qt::AlignTop, drawText);
    p.end();
    heightHint = metrics.height();
    return img;
}

QSize TickerDisplayWidget::sizeHint() const
{
    return QSize(1024, heightHint);
}

void TickerDisplayWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    refreshTickerSettings();
}
