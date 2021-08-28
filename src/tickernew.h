#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);


class TickerNew : public QThread
{
    Q_OBJECT
    Settings settings;
    void run() override;
    bool m_stop{false};
    QPixmap scrollImage;
    QString m_text;
    int m_height{0};
    int m_width{0};
    int m_imgWidth{1024};
    int m_txtWidth{1024};
    int curOffset{0};
    bool m_textOverflows{false};
    int m_speed{5};
    bool m_textChanged{false};
    std::string m_loggingPrefix{"[TickerThread]"};
    std::shared_ptr<spdlog::logger> m_logger;
public:
    TickerNew();
    QSize getSize();
    void stop();
public slots:
    void setTickerGeometry(const int width, const int height);
    void setText(const QString& text);
    void refresh();
    void setSpeed(const int speed);
signals:
    void newFrame(const QPixmap frame);
    void newFrameRect(const QPixmap frame, const QRect displayArea);
    void newRect(const QRect displayArea);
};

class TickerDisplayWidget : public QWidget
{
    Q_OBJECT
    std::string m_loggingPrefix{"[TickerDisplayWidget]"};
    std::shared_ptr<spdlog::logger> m_logger;
    TickerNew *ticker;
public:
        TickerDisplayWidget(QWidget *parent = 0);
        ~TickerDisplayWidget();
        void setText(const QString& newText);
        QSize sizeHint() const;
        void setSpeed(int speed);
        QPixmap m_image;
        QRect drawRect;
        bool rectBasedDrawing{false};
        void stop();
        void setTickerEnabled(bool enabled);
        void refresh() {ticker->refresh();}
        // QWidget interface
protected:
        void resizeEvent(QResizeEvent *event);
private slots:
        void newFrameRect(const QPixmap& frame, const QRect displayArea);
        void newRect(const QRect displayArea);
        void newFrame(const QPixmap& frame);

        // QWidget interface
protected:
        void paintEvent(QPaintEvent *event);
};

#endif // TICKERNEW_H
