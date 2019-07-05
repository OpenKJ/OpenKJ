#include "tickernew.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#include <QResizeEvent>

void TickerNew::run() {
    while (!m_stop)
    {
        if (!m_textOverflows)
            curOffset = 0;
        if (curOffset >= m_txtWidth)
        {
            qInfo() << "TickerNew - Reset offset";
            curOffset = 0;
        }
       // QPixmap frame = scrollImage.copy(curOffset,0,m_width,m_height);
       // emit newFrame(frame);
        emit newFrameRect(scrollImage, QRect(curOffset,0,m_width,m_height));
        curOffset = curOffset + 2;
        msleep(m_speed);
    }

}

void TickerNew::stop()
{
    m_stop = true;
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
    return QSize(m_width, m_height);
}

void TickerNew::setTickerGeometry(int width, int height)
{
    qInfo() << "TickerNew - setTickerGeometry(" << width << "," << height << ") called";
    m_height = height;
    m_width = width;
    scrollImage = QPixmap(width * 2, height);
    setText(m_text);
}

void TickerNew::setText(QString text)
{
    qInfo() << "TickerNew - setText(" << text << ") called";
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
    qInfo() << "TickerNew - drawing text: " << drawText;
    scrollImage.fill(settings->tickerBgColor());
    QPainter p;
    p.begin(&scrollImage);
    p.setPen(QPen(settings->tickerTextColor()));
    p.setFont(settings->tickerFont());
    p.drawText(scrollImage.rect(), Qt::AlignLeft | Qt::AlignTop, drawText);
    p.end();
}

void TickerNew::refresh()
{
    setText(m_text);
}

void TickerNew::setSpeed(int speed)
{
    if (speed > 50)
        m_speed = 50;
    else
        m_speed = 51 - speed;
}

TickerDisplayWidget::TickerDisplayWidget(QWidget *parent)
    : QWidget(parent)
{
    ticker = new TickerNew();
    ticker->setTickerGeometry(this->width(), this->height());
    ticker->start();
    connect(ticker, SIGNAL(newFrame(QPixmap)), this, SLOT(newFrame(QPixmap)));
    connect(ticker, SIGNAL(newFrameRect(QPixmap, QRect)), this, SLOT(newFrameRect(QPixmap, QRect)));
    rectBasedDrawing = false;
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
    rectBasedDrawing = false;
    m_image = frame;
    update();
}

void TickerDisplayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    if (!rectBasedDrawing)
        p.drawPixmap(this->rect(), m_image);
    else {
        p.drawPixmap(this->rect(), m_image, drawRect);
    }
}
