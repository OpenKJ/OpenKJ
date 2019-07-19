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
    bool l_stop  = false;
    while (!l_stop)
    {
        //qInfo() << "Locking mutex in run()";
        //mutex.lock();
        if (!mutex.tryLock(10))
        {
            qWarning() << "TickerNew - run() unable to lock mutex!";
            continue;
        }
        if (!m_textOverflows)
            curOffset = 0;
        if (curOffset >= m_txtWidth)
        {
           // qInfo() << "TickerNew - Reset offset";
            curOffset = 0;
        }
        if (m_stop)
        {
            qInfo() << "Unlocking mutex in run() (stopping)";
            mutex.unlock();
            return;
        }
        emit newFrameRect(scrollImage, QRect(curOffset,0,m_width,m_height));
        curOffset = curOffset + 2;
        l_stop = m_stop;
        //qInfo() << "Unlocking mutex in run()";
        mutex.unlock();
        msleep(m_speed);
    }
}

void TickerNew::stop()
{
    qInfo() << "Locking mutex in stop()";
    if (!mutex.tryLock(10))
    {
        qWarning() << "TickerNew - stop() unable to lock mutex!";
        return;
    }
    //mutex.lock();
    m_stop = true;
    qInfo() << "Unlocking mutex in stop()";
    mutex.unlock();
}

TickerNew::TickerNew()
{
    settings = new Settings(this);
    setText("No ticker data");
    m_stop = false;
    curOffset = 0;
    m_imgWidth = 1024;
    m_txtWidth = 1024;
    m_width = 0;
    m_height = 0;
    m_textOverflows = false;
    m_speed = 5;
}

QSize TickerNew::getSize()
{
    qInfo() << "Locking mutex in getSize()";
    if (!mutex.tryLock(10))
    {
        qWarning() << "TickerNew - getSize() unable to lock mutex!";
        return QSize();
    }
    //mutex.lock();
    QSize size = QSize(m_width, m_height);
    qInfo() << "Unlocking mutex in getSize()";
    mutex.unlock();
    return size;
}

void TickerNew::setTickerGeometry(int width, int height)
{
    qInfo() << "TickerNew - setTickerGeometry(" << width << "," << height << ") called";
    qInfo() << "Locking mutex in setTickerGeometry()";
    if (!mutex.tryLock(10))
    {
        qWarning() << "TickerNew - setTickerGeometry() unable to lock mutex!";
        return;
    }
    //mutex.lock();
    m_height = height;
    m_width = width;
    scrollImage = QPixmap(width * 2, height);
    qInfo() << "Unlocking mutex in setTickerGeometry()";
    mutex.unlock();
    setText(m_text);
    qInfo() << "TickerNew - setTickerGeometry() completed";
}

void TickerNew::setText(QString text)
{
    qInfo() << "TickerNew - setText(" << text << ") called";
    qInfo() << "Locking mutex in setText()";
    if (!mutex.tryLock(10))
    {
        qWarning() << "TickerNew - setText() unable to lock mutex!";
        return;
    }
    //mutex.lock();
    m_textOverflows = false;
    m_text = text;
    QString drawText;
    QFont tickerFont = settings->tickerFont();
    m_imgWidth = QFontMetrics(tickerFont).width(text);
    m_txtWidth = m_imgWidth;
    if (m_imgWidth > m_width)
    {
        m_textOverflows = true;
        drawText.append(text + " | " + text + " | ");
        m_imgWidth = QFontMetrics(tickerFont).width(drawText);
        m_txtWidth = m_txtWidth + QFontMetrics(tickerFont).width(" | ");
        scrollImage = QPixmap(m_imgWidth, m_height);
    }
    else {
        drawText = text;
        scrollImage = QPixmap(m_width, m_height);
    }
    //qInfo() << "TickerNew - drawing text: " << drawText;
    scrollImage.fill(settings->tickerBgColor());
    QPainter p;
    p.begin(&scrollImage);
    p.setPen(QPen(settings->tickerTextColor()));
    p.setFont(settings->tickerFont());
    p.drawText(scrollImage.rect(), Qt::AlignLeft | Qt::AlignTop, drawText);
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
    if (!mutex.tryLock(10))
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
    ticker->setTickerGeometry(this->width(), this->height());
    //ticker->start();
    connect(ticker, SIGNAL(newFrame(QPixmap)), this, SLOT(newFrame(QPixmap)));
    connect(ticker, SIGNAL(newFrameRect(QPixmap, QRect)), this, SLOT(newFrameRect(QPixmap, QRect)));
    rectBasedDrawing = false;
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

void TickerDisplayWidget::newFrameRect(QPixmap frame, QRect displayArea)
{
    rectBasedDrawing = true;
    m_image = frame;
    drawRect = displayArea;
    //qInfo() << "Received drawRect: " << displayArea;
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
    else {
        p.drawPixmap(this->rect(), m_image, drawRect);
    }
}
