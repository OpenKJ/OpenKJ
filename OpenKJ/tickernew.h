#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>




class TickerNew : public QThread
{
    Q_OBJECT
    Settings *settings;
    void run() override;
    bool m_stop;
    QPixmap scrollImage;
    QString m_text;
    int m_height;
    int m_width;
    int m_imgWidth;
    int m_txtWidth;
    int curOffset;
    bool m_textOverflows;
    int m_speed;
public:
    TickerNew();
    QSize getSize();
    void stop();
public slots:
    void setTickerGeometry(int width, int height);
    void setText(QString text);
    void refresh();
    void setSpeed(int speed);
signals:
    void newFrame(QPixmap frame);
    void newFrameRect(QPixmap frame, QRect displayArea);
};

class TickerDisplayWidget : public QWidget
{
    Q_OBJECT
    TickerNew *ticker;
public:
        TickerDisplayWidget(QWidget *parent = 0);
        ~TickerDisplayWidget();
        void setText(const QString &newText);
        QSize sizeHint() const;
        void setSpeed(int speed);
        QPixmap m_image;
        QRect drawRect;
        bool rectBasedDrawing;
        void stop();
        void setTickerEnabled(bool enabled);

        // QWidget interface
protected:
        void resizeEvent(QResizeEvent *event);
private slots:
        void newFrameRect(QPixmap frame, QRect displayArea);
        void newFrame(QPixmap frame);

        // QWidget interface
protected:
        void paintEvent(QPaintEvent *event);
};

#endif // TICKERNEW_H
