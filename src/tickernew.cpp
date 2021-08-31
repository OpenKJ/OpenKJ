#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QMutex>
#include <QApplication>
#include <QTextStream>
#include <utility>
#include <QTimer>

#ifdef _MSC_VER
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>
#endif

void TickerNew::run() {
#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif
    m_logger->info("{} Ticker starting", m_loggingPrefix);
    m_stop = false;
    m_textChanged = true;
    while (!m_stop)
    {
        if (!m_textOverflows)
            curOffset = 0;
        if (curOffset >= m_txtWidth)
        {
            curOffset = 0;
        }
        if (!m_textChanged)
                emit newRect(QRect(curOffset,0,m_width,m_height));
        else
        {
            m_textChanged = false;
            m_mutex.lock();
            emit newFrameRect(scrollImage, QRect(curOffset,0,m_width,m_height));
            m_mutex.unlock();
        }
        curOffset++;
        TickerNew::usleep(m_speed / 2 * 250);
    }
}

void TickerNew::stop()
{
    if (!m_mutex.tryLock(100))
    {
        m_logger->warn("{} stop() unable to lock m_mutex!", m_loggingPrefix);
        return;
    }
    m_stop = true;
    m_mutex.unlock();
}

TickerNew::TickerNew()
{
    m_logger = spdlog::get("logger");
    setText("No ticker data");
    setObjectName("Ticker");
}

QSize TickerNew::getSize()
{
    if (!m_mutex.tryLock(100))
    {
        m_logger->warn("{} getSize() unable to lock m_mutex!", m_loggingPrefix);
        return {};
    }
    QSize size = scrollImage.size();
    m_mutex.unlock();
    return size;
}

void TickerNew::setWidth(int width)
{
    if (!m_mutex.tryLock(100))
    {
        m_logger->warn("{} setWidth() unable to lock m_mutex", m_loggingPrefix);
        return;
    }
#ifdef Q_OS_WIN
    m_height = QFontMetrics(m_settings.tickerFont()).height();
#else
    m_height = static_cast<int>(QFontMetrics(m_settings.tickerFont()).tightBoundingRect("PLACEHOLDERtextgj|i01").height() * 1.2);
#endif
    m_width = width;
    scrollImage = QPixmap(width * 2, m_height);
    //qInfo() << "Unlocking m_mutex in setWidth()";
    m_mutex.unlock();
    setText(m_text);
}

void TickerNew::setText(const QString& text)
{
    if (m_text == text)
        return;
    m_text = text;
    auto imageCreator = new TickerImageCreator(text, m_width);
    connect(imageCreator, &TickerImageCreator::imageCreated, this, &TickerNew::replaceImage);
    connect(imageCreator, &TickerImageCreator::finished, imageCreator, &TickerImageCreator::deleteLater);
    imageCreator->start();
}

void TickerNew::refresh()
{
    setText(m_text);
}

void TickerNew::setSpeed(int speed)
{
    if (!m_mutex.tryLock(100))
    {
        m_logger->warn("{} setSpeed() unable to lock m_mutex!", m_loggingPrefix);
        return;
    }
    if (speed > 50)
        m_speed = 50;
    else
        m_speed = 51 - speed;
    m_mutex.unlock();
}

void TickerNew::replaceImage(const QPixmap &image, int textWidth) {
    if (!m_mutex.tryLock(1000))
    {
        m_logger->error("{} setText() unable to lock m_mutex!", m_loggingPrefix);
        return;
    }
    m_textChanged = true;
    m_textOverflows = false;
    m_height = image.height();
    image.width();
    if (image.width() > m_width)
        m_textOverflows = true;
    scrollImage = image;
    m_txtWidth = textWidth;
    m_mutex.unlock();
}

TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
        : QWidget(parent)
{
    m_logger = spdlog::get("logger");
    ticker = new TickerNew();
    ticker->setWidth(this->width());
    connect(ticker, &TickerNew::newFrame, this, &TickerDisplayWidget::newFrame);
    connect(ticker, &TickerNew::newFrameRect, this, &TickerDisplayWidget::newFrameRect);
    connect(ticker, &TickerNew::newRect, this, &TickerDisplayWidget::newRect);
}

TickerDisplayWidget::~TickerDisplayWidget()
{
    ticker->stop();
    ticker->wait(1000);
    delete ticker;
}

void TickerDisplayWidget::setText(const QString& newText)
{
    ticker->setText(newText);
    setFixedHeight(ticker->getSize().height());
}

QSize TickerDisplayWidget::sizeHint() const
{
    return ticker->getSize();
}

void TickerDisplayWidget::setSpeed(int speed)
{
    ticker->setSpeed(speed);
}

void TickerDisplayWidget::stop()
{
    ticker->stop();
}

void TickerDisplayWidget::setTickerEnabled(bool enabled)
{
    m_logger->info("{} Enabled set to: {}", m_loggingPrefix, enabled);
    if (enabled && !ticker->isRunning()) {
        ticker->start();
        ticker->setPriority(QThread::TimeCriticalPriority);
    }
    else if (!enabled && ticker->isRunning())
        ticker->stop();
}


void TickerDisplayWidget::resizeEvent(QResizeEvent *event)
{
    ticker->setWidth(event->size().width());
}

void TickerDisplayWidget::newFrameRect(const QPixmap& frame, const QRect displayArea)
{
    rectBasedDrawing = true;
    m_image = frame;
    drawRect = displayArea;
    update();
}

void TickerDisplayWidget::newRect(const QRect displayArea)
{
    if (!isVisible())
        return;
    rectBasedDrawing = true;
    drawRect = displayArea;
    update();
}

void TickerDisplayWidget::newFrame(const QPixmap& frame)
{
    if (!isVisible())
        return;
    rectBasedDrawing = false;
    m_image = frame;
    update();
}

void TickerDisplayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (!isVisible())
        return;
    QPainter p(this);
    if (!rectBasedDrawing)
        p.drawPixmap(this->rect(), m_image);
    else
        p.drawPixmap(this->rect(), m_image, drawRect);
}

void TickerImageCreator::run() {
    Settings settings;
    std::string m_loggingPrefix{"[TickerImageCreator]"};
    std::shared_ptr<spdlog::logger> m_logger = spdlog::get("logger");
    m_logger->trace("{} Thread starting up", m_loggingPrefix);
    auto st = std::chrono::high_resolution_clock::now();

    m_logger->info("{} Rendering ticker text: {}", m_loggingPrefix, m_tickerText);
    QFont tickerFont = settings.tickerFont();
    QPixmap img;
    int imgHeight;
    int imgWidth;
    int txtWidth;
    if (m_tickerText == "")
        m_tickerText = "Placeholder - ticker was set to empty string";
#ifdef Q_OS_WIN
    imgHeight = QFontMetrics(tickerFont).height();
#else
    imgHeight = QFontMetrics(tickerFont).size(Qt::TextSingleLine, m_tickerText).height();
#endif
    imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, m_tickerText).width();
    txtWidth = imgWidth;
    QString drawText;
    if (imgWidth > m_targetWidth)
    {
        drawText.append(m_tickerText + " • " + m_tickerText + " • ");
        imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, drawText).width();
        txtWidth = txtWidth + QFontMetrics(tickerFont).size(Qt::TextSingleLine," • ").width();
        img = QPixmap(imgWidth, imgHeight);
    }
    else {
        drawText = m_tickerText;
        img = QPixmap(imgWidth, imgHeight);
    }
    img.fill(settings.tickerBgColor());
    QPainter p;
    p.begin(&img);
    p.setPen(QPen(settings.tickerTextColor()));
    p.setFont(settings.tickerFont());
    p.drawText(img.rect(), Qt::AlignLeft | Qt::AlignVCenter, drawText);
    p.end();
    if (settings.auxTickerFile() != QString())
    {
        m_logger->debug("{} Saving ticker data to aux file: {}", m_loggingPrefix, settings.auxTickerFile());
        QFile auxFile(settings.auxTickerFile());
        auxFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&auxFile);
        out << drawText;
        auxFile.close();
    }
    emit imageCreated(img, txtWidth);
    m_logger->trace("{} Ticker image rendered in {}ms",
                    m_loggingPrefix,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
}

TickerImageCreator::TickerImageCreator(QString TickerText, int targetWidth) : m_tickerText(std::move(TickerText)), m_targetWidth(targetWidth)
{

}
