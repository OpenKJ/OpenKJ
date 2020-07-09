#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>
#include <QGraphicsView>
#include <QVariantAnimation>
#include <QOpenGLWidget>
#include <QTimer>

class TickerDisplayWidget : public QGraphicsView
{
    Q_OBJECT
public:
        TickerDisplayWidget(QWidget *parent = 0);
        ~TickerDisplayWidget();
        void setText(const QString &newText);
        void setSpeed(int speed);
        QPixmap m_image;
        QPixmap m_image2;
        void stop();
        void setTickerEnabled(const bool &enabled);
        void refreshTickerSettings();

private:
        QPixmap getPixmapFromString(const QString &text);
        int heightHint;
        QString currentTxt;
        QVariantAnimation *animation;
        QGraphicsPixmapItem* spm;
        QGraphicsPixmapItem* spm2;
        QGraphicsScene *scene;
        Settings settings;
        QOpenGLWidget *glWidget;
        QTimer timer;
        bool underflow;
        bool flipped{false};
        int speed;
        qreal pixelShift;
        int jumpPoint;

private slots:


        // QWidget interface
public:
        QSize sizeHint() const;

        // QWidget interface
protected:
        void resizeEvent(QResizeEvent *event);
};

#endif // TICKERNEW_H
