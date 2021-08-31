#ifndef TICKERNEW_H
#define TICKERNEW_H

#include <QObject>
#include <QPixmap>
#include <QThread>
#include <settings.h>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>
#include <QMutex>

std::ostream& operator<<(std::ostream& os, const QString& s);

class TickerImageCreator : public QThread {
Q_OBJECT
    QString m_tickerText;
    int m_targetWidth;
    void run() override;

public:
    TickerImageCreator(QString TickerText, int targetWidth);

signals:
    void imageCreated(QPixmap image, int textWidth);

};

class TickerNew : public QThread
{
Q_OBJECT
private:
    QMutex m_mutex;
    Settings m_settings;
    bool m_stop{false};
    QPixmap scrollImage;
    QString m_text;
    int m_height{0};
    int m_width{0};
    int m_txtWidth{1024};
    int curOffset{0};
    bool m_textOverflows{false};
    int m_speed{5};
    bool m_textChanged{false};
    std::string m_loggingPrefix{"[TickerThread]"};
    std::shared_ptr<spdlog::logger> m_logger;

    void run() override;

public:
    TickerNew();
    QSize getSize();
    void stop();

public slots:
    void setWidth(int width);
    void setText(const QString& text);
    void replaceImage(const QPixmap &image, int textWidth);
    void refresh();
    void setSpeed(int speed);

signals:
    void newFrame(QPixmap frame);
    void newFrameRect(QPixmap frame, QRect displayArea);
    void newRect(QRect displayArea);
};

class TickerDisplayWidget : public QWidget
{
Q_OBJECT
private:
    std::string m_loggingPrefix{"[TickerDisplayWidget]"};
    std::shared_ptr<spdlog::logger> m_logger;
    TickerNew *ticker;
    QPixmap m_image;
    QRect drawRect;

public:
    explicit TickerDisplayWidget(QWidget *parent = nullptr);
    ~TickerDisplayWidget() override;
    void setText(const QString& newText);
    [[nodiscard]] QSize sizeHint() const override;
    void setSpeed(int speed);

    bool rectBasedDrawing{false};
    void stop();
    void setTickerEnabled(bool enabled);
    void refresh() {ticker->refresh();}

private slots:
    void newFrameRect(const QPixmap& frame, QRect displayArea);
    void newRect(QRect displayArea);
    void newFrame(const QPixmap& frame);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

#endif // TICKERNEW_H