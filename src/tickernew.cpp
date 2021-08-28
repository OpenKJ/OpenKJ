#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QMutex>
#include <QApplication>
#include <QTextStream>

QMutex mutex;


void TickerNew::run() {
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
            mutex.lock();
            emit newFrameRect(scrollImage, QRect(curOffset,0,m_width,m_height));
            mutex.unlock();
        }
        curOffset++;
        TickerNew::usleep(m_speed / 2 * 250);
    }
}

void TickerNew::stop()
{
    if (!mutex.tryLock(100))
    {
        m_logger->warn("{} stop() unable to lock mutex!", m_loggingPrefix);
        return;
    }
    m_stop = true;
    mutex.unlock();
}

TickerNew::TickerNew()
{
    m_logger = spdlog::get("logger");
    setText("No ticker data");
    setObjectName("Ticker");
}

QSize TickerNew::getSize()
{
    if (!mutex.tryLock(100))
    {
        m_logger->warn("{} getSize() unable to lock mutex!", m_loggingPrefix);
        return {};
    }
    QSize size = scrollImage.size();
    mutex.unlock();
    return size;
}

void TickerNew::setTickerGeometry(const int width, const int height)
{
    if (!mutex.tryLock(100))
    {
        m_logger->warn("{} setTickerGeometry() unable to lock mutex", m_loggingPrefix);
        return;
    }
#ifdef Q_OS_WIN
    m_height = QFontMetrics(settings.tickerFont()).height();
#else
    m_height = QFontMetrics(settings.tickerFont()).tightBoundingRect("PLACEHOLDERtextgj|i01").height() * 1.2;
#endif
    m_width = width;
    scrollImage = QPixmap(width * 2, m_height);
    //qInfo() << "Unlocking mutex in setTickerGeometry()";
    mutex.unlock();
    setText(m_text);
}

void TickerNew::setText(const QString& text)
{
    if (text == m_text)
        return;
    m_logger->trace("{} [{}] Called", m_loggingPrefix, __func__);
    auto st = std::chrono::high_resolution_clock::now();
    m_logger->info("{} Setting ticker text: {}", m_loggingPrefix, text);
    if (!mutex.tryLock(100))
    {
        m_logger->warn("{} setText() unable to lock mutex!", m_loggingPrefix);
        return;
    }
    //mutex.lock();
    m_textChanged = true;
    m_textOverflows = false;
    m_text = text;
    QString drawText;
    QFont tickerFont = settings.tickerFont();
#ifdef Q_OS_WIN
    m_height = QFontMetrics(tickerFont).height();
#else
    m_height = QFontMetrics(tickerFont).tightBoundingRect(text).height() * 1.2;
#endif
    m_imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, text).width();
    m_txtWidth = m_imgWidth;
    if (m_imgWidth > m_width)
    {
        m_textOverflows = true;
        drawText.append(text + " • " + text + " • ");
        m_imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, drawText).width();
        m_txtWidth = m_txtWidth + QFontMetrics(tickerFont).size(Qt::TextSingleLine," • ").width();
        scrollImage = QPixmap(m_imgWidth, m_height);
    }
    else {
        drawText = text;
        scrollImage = QPixmap(m_width, m_height);
    }
    scrollImage.fill(settings.tickerBgColor());
    QPainter p;
    p.begin(&scrollImage);
    p.setPen(QPen(settings.tickerTextColor()));
    p.setFont(settings.tickerFont());
    p.drawText(scrollImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, drawText);
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
    mutex.unlock();
    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
    //qInfo() << "TickerNew - setText() completed";
}

void TickerNew::refresh()
{
    setText(m_text);
}

void TickerNew::setSpeed(int speed)
{
    //qInfo() << "Locking mutex in setSpeed()";
    if (!mutex.tryLock(100))
    {
        m_logger->warn("{} setSpeed() unable to lock mutex!", m_loggingPrefix);
        return;
    }
    //mutex.lock();
    if (speed > 50)
        m_speed = 50;
    else
        m_speed = 51 - speed;
    mutex.unlock();
}

TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    m_logger = spdlog::get("logger");
    ticker = new TickerNew();
    ticker->setTickerGeometry(this->width(), this->height());
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
    ticker->setTickerGeometry(event->size().width(), event->size().height());
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
