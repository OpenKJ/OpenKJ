#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>
#include <QOpenGLWidget>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <settings.h>


TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QGraphicsView(parent)
{

    heightHint = 100;


    QThread::currentThread()->setPriority(QThread::HighPriority);

    setViewport(new QOpenGLWidget());
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scene = new QGraphicsScene(this);
    //scene->setSceneRect(ui->graphicsView->rect());





    setScene(scene);
    setRenderHints(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheNone);

    spm = new QGraphicsPixmapItem(getPixmapFromString("Placeholder txt"));
    scene->addItem(spm);
    spm->setPos(QPointF(rect().left(), 0.0f));
    animation = new QVariantAnimation(this);
    int duration = 4.0f * (float)spm->boundingRect().width();
    qWarning() << "duration " << duration;
    animation->setDuration(duration);
    animation->setStartValue(QPointF(0.0f, 0.0f));
    animation->setEndValue(QPointF(-spm->boundingRect().width(), 0.0f));
    animation->setLoopCount(-1);

    QObject::connect(animation, &QVariantAnimation::valueChanged, [this](const QVariant &value){
        int halfway = spm->pixmap().width() / 2;
        if (value.toPoint().x() <= -halfway)
        {
            spm->setPos(QPointF(0.0f,0.0f));
            animation->stop();
            animation->setStartValue(QPointF(0.0f, 0.0f));
            animation->setEndValue(QPointF(-spm->boundingRect().width(), 0.0f));
            animation->setLoopCount(-1);
            animation->start();
        }
        spm->setPos(value.toPointF());
        //qWarning() << "fired";
    });

    animation->start();
}

TickerDisplayWidget::~TickerDisplayWidget()
{

}

void TickerDisplayWidget::setText(const QString &newText)
{
    animation->stop();
    delete animation;
    scene->removeItem(spm);
    delete spm;
    currentTxt = newText;

    spm = new QGraphicsPixmapItem(getPixmapFromString(currentTxt));
    scene->addItem(spm);
    spm->setPos(QPointF(rect().left(), 0.0f));
    animation = new QVariantAnimation(this);
    int duration = 4.0f * (float)spm->boundingRect().width();
    qWarning() << "duration " << duration;
    animation->setDuration(duration);
    animation->setStartValue(QPointF(0.0f, 0.0f));
    animation->setEndValue(QPointF(-spm->boundingRect().width(), 0.0f));
    animation->setLoopCount(-1);

    QObject::connect(animation, &QVariantAnimation::valueChanged, [this](const QVariant &value){
        int halfway = spm->pixmap().width() / 2;
        if (value.toPoint().x() < -halfway)
        {
            spm->setPos(QPointF(0.0f,0.0f));
            animation->stop();
            animation->setStartValue(QPointF(0.0f, 0.0f));
            animation->setEndValue(QPointF(-spm->boundingRect().width(), 0.0f));
            animation->setLoopCount(-1);
            animation->start();
        }
        spm->setPos(value.toPointF());
        //qWarning() << "fired";
    });

    animation->start();
}


void TickerDisplayWidget::setSpeed(int speed)
{
}

void TickerDisplayWidget::stop()
{
}

void TickerDisplayWidget::setTickerEnabled(bool enabled)
{

}

QPixmap TickerDisplayWidget::getPixmapFromString(QString text)
{
    QString m_text = text;
    QString drawText;
    drawText = text + " - " + text;
    QFont tickerFont = settings.tickerFont();
    tickerFont.setPointSize(36);
    int m_imgWidth = QFontMetrics(tickerFont).horizontalAdvance(drawText);
    //drawText = text;
    QPixmap img = QPixmap(m_imgWidth, QFontMetrics(tickerFont).boundingRect(drawText).height() + 10);
    //qInfo() << "TickerNew - drawing text: " << drawText;
    img.fill(Qt::black);
    QPainter p;
    p.begin(&img);
    p.setPen(QPen(Qt::yellow));
    p.setFont(tickerFont);
    p.drawText(img.rect().adjusted(0,3,0,3), Qt::AlignLeft | Qt::AlignTop, drawText);
    p.end();
    heightHint = QFontMetrics(tickerFont).height();

    return img;
}

QSize TickerDisplayWidget::sizeHint() const
{
    return QSize(1024, heightHint);
}
