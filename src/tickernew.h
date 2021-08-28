#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>
#include <QTimer>

std::ostream& operator<<(std::ostream& os, const QString& s);


class TickerNew : public QObject
{
    Q_OBJECT
    Settings settings;
    QPixmap m_scrollImage;
    QString m_text;
    int m_height{0};
    int m_width{0};
    int m_imgWidth{1024};
    int m_txtWidth{1024};
    int m_curOffset{0};
    bool m_textOverflows{false};
    int m_speed{5};
    bool m_textChanged{false};
    std::string m_loggingPrefix{"[TickerThread]"};
    std::shared_ptr<spdlog::logger> m_logger;
    QTimer *m_timer;
public:
    explicit TickerNew(QObject *parent = nullptr);
    QSize getSize();
private slots:
    void timerTimeout();
public slots:
    void setTickerGeometry(int width, int height);
    void setText(const QString &text, bool force);
    void refresh();
    void setSpeed(int speed);
    void start();
    void stop();

signals:
    void newFrame(QPixmap frame);
    void newFrameRect(QPixmap frame, QRect displayArea);
    void newRect(QRect displayArea);
};

class TickerDisplayWidget : public QWidget
{
    Q_OBJECT
    std::string m_loggingPrefix{"[TickerDisplayWidget]"};
    std::shared_ptr<spdlog::logger> m_logger;
    TickerNew *ticker;
    QThread *worker;
public:
        explicit TickerDisplayWidget(QWidget *parent = nullptr);
        ~TickerDisplayWidget() override;
        [[nodiscard]] QSize sizeHint() const override;
        void setSpeed(int speed);
        QPixmap m_image;
        QRect drawRect;
        bool rectBasedDrawing{false};
        void stop();
        void setTickerEnabled(bool enabled);
        void refresh() {ticker->refresh();}
        // QWidget interface
protected:
        void resizeEvent(QResizeEvent *event) override;
public slots:
    void setText(const QString& newText);
private slots:
        void newFrameRect(QPixmap frame, QRect displayArea);
        void newRect(QRect displayArea);
        void newFrame(QPixmap frame);

        // QWidget interface
protected:
        void paintEvent(QPaintEvent *event) override;

        signals:
    void setTextSignal(const QString &newText, bool force);
    void setTickerGeometrySignal(int width, int height);
    void speedChanged(int speed);
};

#endif // TICKERNEW_H
