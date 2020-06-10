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
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHints(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheNone);
    spm = new QGraphicsPixmapItem(getPixmapFromString("Placeholder txt"));
    scene->addItem(spm);
    spm->setPos(QPointF(0.0f, 0.0f));
    timer = new QTimer(this);
    timer->setInterval(4);
    qreal pixelShift = 2.2;
    QObject::connect(timer, &QTimer::timeout, [this,pixelShift]() {
       if (underflow)
       {
           spm->setPos(0.0f,0.0f);
           return;
       }
       qreal halfway = spm->pixmap().width() / 2.0f;
       qreal xpos = spm->pos().x();
       if (xpos <= -halfway)
       {
            spm->setPos(0.0f,0.0f);
       }
       else
       {
            spm->setPos(QPointF(xpos - pixelShift, 0.0f));
       }

    });
    timer->start();
}

TickerDisplayWidget::~TickerDisplayWidget()
{

}

void TickerDisplayWidget::setText(const QString &newText)
{
    currentTxt = newText;
    timer->stop();
    scene->removeItem(spm);
    delete spm;
    spm = new QGraphicsPixmapItem(getPixmapFromString(newText));
    scene->addItem(spm);
    spm->setPos(QPointF(rect().left(), 0.0f));

    timer->start();
}


void TickerDisplayWidget::setSpeed(int speed)
{
}

void TickerDisplayWidget::stop()
{
    timer->stop();
}

void TickerDisplayWidget::setTickerEnabled(const bool& enabled)
{
    if (enabled)
        timer->start();
    else
        timer->stop();
}

void TickerDisplayWidget::refreshTickerSettings()
{
    setText(currentTxt);
   // setBackgroundBrush(QBrush(settings.tickerBgColor()));
   // setAutoFillBackground(true);
}

QPixmap TickerDisplayWidget::getPixmapFromString(const QString& text)
{
    int myWidth = this->visibleRegion().boundingRect().width();
    QFont tickerFont = settings.tickerFont();
    QFontMetrics metrics = QFontMetrics(tickerFont);
    QString drawText;
    int pxWidth;
    if (myWidth >= metrics.horizontalAdvance(text))
    {
        pxWidth = myWidth * 2;
        underflow = true;
        drawText = text;

    }
    else
    {
        drawText = text + " | " + text;
        pxWidth = metrics.horizontalAdvance(drawText);
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
