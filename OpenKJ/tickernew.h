#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>
#include <QGraphicsView>
#include <QVariantAnimation>

class TickerDisplayWidget : public QGraphicsView
{
    Q_OBJECT
public:
        TickerDisplayWidget(QWidget *parent = 0);
        ~TickerDisplayWidget();
        void setText(const QString &newText);
        void setSpeed(int speed);
        QPixmap m_image;
        void stop();
        void setTickerEnabled(bool enabled);

private:
        QPixmap getPixmapFromString(QString text);
        int heightHint;
        QString currentTxt;
        QVariantAnimation *animation;
        QGraphicsPixmapItem* spm;
        QGraphicsScene *scene;
        Settings settings;

private slots:


        // QWidget interface
public:
        QSize sizeHint() const;
};

#endif // TICKERNEW_H
