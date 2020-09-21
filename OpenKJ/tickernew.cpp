#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#include <QResizeEvent>
#include <QMutex>
#include <QApplication>

QMutex mutex;


void TickerNew::run() {
    qInfo() << "TickerNew - run() called, ticker starting";
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
        this->usleep(m_speed / 2 * 250);
    }
}

void TickerNew::stop()
{
    qInfo() << "Locking mutex in stop()";
    if (!mutex.tryLock(100))
    {
        qWarning() << "TickerNew - stop() unable to lock mutex!";
        return;
    }
    m_stop = true;
    qInfo() << "Unlocking mutex in stop()";
    mutex.unlock();
}

TickerNew::TickerNew()
{
    setText("No ticker data");
}

const QSize TickerNew::getSize()
{
    qInfo() << "Locking mutex in getSize()";
    if (!mutex.tryLock(100))
    {
        qWarning() << "TickerNew - getSize() unable to lock mutex!";
        return QSize();
    }
    //mutex.lock();
    QSize size = scrollImage.size();
    qInfo() << "Unlocking mutex in getSize()";
    mutex.unlock();
    return size;
}

void TickerNew::setTickerGeometry(const int width, const int height)
{
    qInfo() << "TickerNew - setTickerGeometry(" << width << "," << height << ") called";
    qInfo() << "Locking mutex in setTickerGeometry()";
    if (!mutex.tryLock(100))
    {
        qWarning() << "TickerNew - setTickerGeometry() unable to lock mutex!";
        return;
    }
#ifdef Q_OS_WIN
    m_height = QFontMetrics(settings.tickerFont()).height();
#else
    m_height = QFontMetrics(settings.tickerFont()).tightBoundingRect("PLACEHOLDERtextgj|i01").height() * 1.2;
#endif
    m_width = width;
    scrollImage = QPixmap(width * 2, m_height);
    qInfo() << "Unlocking mutex in setTickerGeometry()";
    mutex.unlock();
    setText(m_text);
    qInfo() << "TickerNew - setTickerGeometry() completed";
}

void TickerNew::setText(QString text)
{
    qInfo() << "TickerNew - setText(" << text << ") called";
    qInfo() << "Locking mutex in setText()";
    if (!mutex.tryLock(100))
    {
        qWarning() << "TickerNew - setText() unable to lock mutex!";
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
    m_imgWidth = QFontMetrics(tickerFont).width(text);
    m_txtWidth = m_imgWidth;
    if (m_imgWidth > m_width)
    {
        m_textOverflows = true;
        drawText.append(text + " • " + text + " • ");
        m_imgWidth = QFontMetrics(tickerFont).width(drawText);
        m_txtWidth = m_txtWidth + QFontMetrics(tickerFont).width(" • ");
        scrollImage = QPixmap(m_imgWidth, m_height);
    }
    else {
        drawText = text;
        scrollImage = QPixmap(m_width, m_height);
    }
    //qInfo() << "TickerNew - drawing text: " << drawText;
    scrollImage.fill(settings.tickerBgColor());
    QPainter p;
    p.begin(&scrollImage);
    p.setPen(QPen(settings.tickerTextColor()));
    p.setFont(settings.tickerFont());
    p.drawText(scrollImage.rect(), Qt::AlignLeft | Qt::AlignVCenter, drawText);
    p.end();
    qInfo() << "Unlocking mutex in setText()";
    mutex.unlock();
    qInfo() << "TickerNew - setText() completed";
}

void TickerNew::refresh()
{
    setText(m_text);
}

void TickerNew::setSpeed(int speed)
{
    qInfo() << "Locking mutex in setSpeed()";
    if (!mutex.tryLock(100))
    {
        qWarning() << "TickerNew - setSpeed() unable to lock mutex!";
        return;
    }
    //mutex.lock();
    if (speed > 50)
        m_speed = 50;
    else
        m_speed = 51 - speed;
    qInfo() << "Unlocking mutex in setSpeed()";
    mutex.unlock();
}

TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    ticker = new TickerNew();
    ticker->setPriority(QThread::TimeCriticalPriority);
    ticker->setTickerGeometry(this->width(), this->height());
    connect(ticker, SIGNAL(newFrame(QPixmap)), this, SLOT(newFrame(QPixmap)));
    connect(ticker, SIGNAL(newFrameRect(QPixmap, QRect)), this, SLOT(newFrameRect(QPixmap, QRect)));
    connect(ticker, SIGNAL(newRect(QRect)), this, SLOT(newRect(QRect)));
}

TickerDisplayWidget::~TickerDisplayWidget()
{
    qInfo() << "TickerDisplayWidget destructor called";
    ticker->stop();
    ticker->wait(1000);
    delete ticker;
}

void TickerDisplayWidget::setText(const QString &newText)
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
    qInfo() << "TickerDisplayWidget - setTickerEnabled(" << enabled << ") called";
    if (enabled && !ticker->isRunning())
        ticker->start();
    else if (!enabled && ticker->isRunning())
        ticker->stop();
}


void TickerDisplayWidget::resizeEvent(QResizeEvent *event)
{
    ticker->setTickerGeometry(event->size().width(), event->size().height());
}

void TickerDisplayWidget::newFrameRect(const QPixmap frame, const QRect displayArea)
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

void TickerDisplayWidget::newFrame(const QPixmap frame)
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
