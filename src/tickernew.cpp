#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QMutex>
#include <QApplication>
#include <QTextStream>
#include <utility>


void TickerNew::stop()
{
    m_timer->stop();
}

TickerNew::TickerNew(QObject *parent) : QObject(parent)
{
    m_logger = spdlog::get("logger");
    m_timer = new QTimer(this);
    QTimer::singleShot(250, [&] () {
        setText("No ticker data", false);
    });
    setObjectName("Ticker");
    connect(m_timer, &QTimer::timeout, this, &TickerNew::timerTimeout);
    m_timer->setInterval(m_speed);
}

QSize TickerNew::getSize()
{
    return m_scrollImage.size();
}

void TickerNew::setTickerGeometry(const int width, const int height)
{
#ifdef Q_OS_WIN
    m_height = QFontMetrics(settings.tickerFont()).height();
#else
    m_height = QFontMetrics(settings.tickerFont()).tightBoundingRect("PLACEHOLDERtextgj|i01").height() * 1.2;
#endif
    m_width = width;
    m_scrollImage = QPixmap(width * 2, m_height);
    setText(m_text, true);
}

void TickerNew::setText(const QString &text, bool force)
{
    if (text == m_text && !force)
        return;
    m_logger->trace("{} [{}] Called", m_loggingPrefix, __func__);
    auto st = std::chrono::high_resolution_clock::now();
    m_logger->info("{} Setting ticker text: {}", m_loggingPrefix, text);
    m_textChanged = true;
    m_textOverflows = false;
    m_text = text;
    QString drawText;
    QFont tickerFont = settings.tickerFont();
#ifdef Q_OS_WIN
    m_height = QFontMetrics(tickerFont).height();
#else
   // m_height = QFontMetrics(tickerFont).tightBoundingRect(text).height() * 1.2;
    m_height = QFontMetrics(tickerFont).size(Qt::TextSingleLine, drawText).height();
#endif
    m_imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, text).width();
    m_txtWidth = m_imgWidth;
    if (m_imgWidth > m_width)
    {
        m_textOverflows = true;
        drawText.append(text + " • " + text + " • ");
        m_imgWidth = QFontMetrics(tickerFont).size(Qt::TextSingleLine, drawText).width();
        m_txtWidth = m_txtWidth + QFontMetrics(tickerFont).size(Qt::TextSingleLine," • ").width();
        m_scrollImage = QPixmap(m_imgWidth, m_height);
    }
    else {
        drawText = text;
        m_scrollImage = QPixmap(m_width, m_height);
    }
    m_scrollImage.fill(settings.tickerBgColor());
    QPainter p;
    p.begin(&m_scrollImage);
    p.setPen(QPen(settings.tickerTextColor()));
    p.setFont(tickerFont);
    p.drawText(m_scrollImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, drawText);
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
    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
    //qInfo() << "TickerNew - setText() completed";
}

void TickerNew::refresh()
{
    setText(m_text, false);
}

void TickerNew::setSpeed(int speed)
{
    //qInfo() << "Locking mutex in setSpeed()";
    if (speed > 50)
        speed = 50;
    speed = 55 - speed;
    m_speed = (speed / 2 * 250) / 1000;
    m_speed = std::clamp(m_speed, 1, 7);
    m_timer->setInterval(m_speed);
    m_logger->warn("{} m_timer interval: {}", m_logger, m_timer->interval());
}

void TickerNew::start() {
    m_timer->start();
}

void TickerNew::timerTimeout() {
    if (!m_textOverflows)
        m_curOffset = 0;
    if (m_curOffset >= m_txtWidth)
    {
        m_curOffset = 0;
    }
    if (!m_textChanged)
            emit newRect(QRect(m_curOffset, 0, m_width, m_height));
    else
    {
        m_textChanged = false;
        emit newFrameRect(m_scrollImage, QRect(m_curOffset, 0, m_width, m_height));
    }
    m_curOffset++;
}

TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    m_logger = spdlog::get("logger");
    ticker = new TickerNew();
    worker = new QThread(this);
    worker->setObjectName("TickerWorker");
    worker->start();
    ticker->moveToThread(worker);
    ticker->setTickerGeometry(this->width(), this->height());
    connect(ticker, &TickerNew::newFrame, this, &TickerDisplayWidget::newFrame, Qt::QueuedConnection);
    connect(ticker, &TickerNew::newFrameRect, this, &TickerDisplayWidget::newFrameRect, Qt::QueuedConnection);
    connect(ticker, &TickerNew::newRect, this, &TickerDisplayWidget::newRect, Qt::QueuedConnection);
    connect(this, &TickerDisplayWidget::setTextSignal, ticker, &TickerNew::setText, Qt::QueuedConnection);
    connect(this, &TickerDisplayWidget::setTickerGeometrySignal, ticker, &TickerNew::setTickerGeometry, Qt::QueuedConnection);
    connect(this, &TickerDisplayWidget::speedChanged, ticker, &TickerNew::setSpeed, Qt::QueuedConnection);
    connect(worker, &QThread::finished, ticker, &TickerNew::deleteLater);
}

TickerDisplayWidget::~TickerDisplayWidget()
{
    worker->quit();
    worker->wait();
}

void TickerDisplayWidget::setText(const QString& newText)
{
    m_logger->trace("{} [{}] Called", m_loggingPrefix, __func__);
    auto st = std::chrono::high_resolution_clock::now();
    emit setTextSignal(newText, false);
    setFixedHeight(ticker->getSize().height());
    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
}

QSize TickerDisplayWidget::sizeHint() const
{
    return m_image.size();
}

void TickerDisplayWidget::setSpeed(int speed)
{
    emit speedChanged(speed);
}

void TickerDisplayWidget::stop()
{
    QMetaObject::invokeMethod(ticker, &TickerNew::stop, Qt::QueuedConnection);
}

void TickerDisplayWidget::setTickerEnabled(bool enabled)
{
    m_logger->info("{} Enabled set to: {}", m_loggingPrefix, enabled);
    if (enabled)
        QMetaObject::invokeMethod(ticker, &TickerNew::start, Qt::QueuedConnection);
    else
        QMetaObject::invokeMethod(ticker, &TickerNew::stop, Qt::QueuedConnection);

}


void TickerDisplayWidget::resizeEvent(QResizeEvent *event)
{
    emit setTickerGeometrySignal(event->size().width(), event->size().height());
}

void TickerDisplayWidget::newFrameRect(QPixmap frame, QRect displayArea)
{
    rectBasedDrawing = true;
    m_image = std::move(frame);
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

void TickerDisplayWidget::newFrame(QPixmap frame)
{
    if (!isVisible())
        return;
    rectBasedDrawing = false;
    m_image = std::move(frame);
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
