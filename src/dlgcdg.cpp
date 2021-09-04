/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dlgcdg.h"
#include "ui_dlgcdg.h"
#include <QDesktopWidget>
#include <QSvgRenderer>
#include <QPainter>
#include <QDir>
#include <QImageReader>
#include <QScreen>


VideoDisplay *DlgCdg::getVideoDisplay()
{
    return ui->videoDisplayKar;
}

VideoDisplay *DlgCdg::getVideoDisplayBm()
{
    return ui->videoDisplayBm;
}

DlgCdg::DlgCdg(MediaBackend &KaraokeBackend, MediaBackend &BreakBackend, QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f), ui(new Ui::DlgCdg), m_kmb(KaraokeBackend), m_bmb(BreakBackend)
{
    ui->setupUi(this);
    m_tWidget = std::make_unique<TransparentWidget>(this);
    m_tWidget->setObjectName("DurationTimer");
    m_tWidget->show();
    m_tWidget->move(m_settings.durationPosition());
    ui->videoDisplayKar->setFillOnPaint(false);
    ui->widgetAlert->setAutoFillBackground(true);
    ui->fsToggleWidget->hide();
    ui->widgetAlert->hide();
    ui->widgetAlert->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->widgetAlert->setMouseTracking(true);
    ui->scroll->setVisible(m_settings.tickerEnabled());
    ui->scroll->setTickerEnabled(m_settings.tickerEnabled());
    m_tWidget->setVisible(m_settings.cdgRemainEnabled());
    tickerFontChanged();
    ui->scroll->setSpeed(m_settings.tickerSpeed());
    m_tWidget->setTextFont(m_settings.cdgRemainFont());
    QPalette palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), m_settings.tickerTextColor());
    ui->scroll->setPalette(palette);
    palette = this->palette();
    palette.setColor(QPalette::Window, m_settings.tickerBgColor());
    setPalette(palette);
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
    m_fullScreen = m_settings.cdgWindowFullscreen();
    m_tWidget->setTextColor(m_settings.cdgRemainTextColor());
    m_tWidget->setBackgroundColor(m_settings.cdgRemainBgColor());
    applyBackgroundImageMode();
    showAlert(false);
    alertFontChanged(m_settings.karaokeAAAlertFont());
    alertBgColorChanged(m_settings.alertBgColor());
    alertTxtColorChanged(m_settings.alertTxtColor());
    cdgOffsetsChanged();
    connect(ui->videoDisplayKar, &VideoDisplay::mouseMoveEvent, this, &DlgCdg::mouseMove);
    connect(ui->btnToggleFullscreen, &QPushButton::clicked, this, &DlgCdg::btnToggleFullscreenClicked);
    connect(&m_timerSlideShow, &QTimer::timeout, this, &DlgCdg::timerSlideShowTimeout);
    connect(&m_timer1s, &QTimer::timeout, this, &DlgCdg::timer1sTimeout);
    connect(&m_timerAlertCountdown, &QTimer::timeout, this, &DlgCdg::timerCountdownTimeout);
    connect(&m_timerButtonShow, &QTimer::timeout, [&] () { ui->fsToggleWidget->hide(); });
    m_timerButtonShow.setInterval(1000);
    m_timerAlertCountdown.setInterval(1000);
    m_timer1s.start(1000);
    if (m_settings.bgMode() == Settings::BG_MODE_SLIDESHOW)
        m_timerSlideShow.start(static_cast<int>(m_settings.slideShowInterval() * 1000));
    ui->videoDisplayBm->hide();
    if (!m_settings.showCdgWindow())
        hide();
    else
        show();
}

DlgCdg::~DlgCdg() = default;

void DlgCdg::setTickerText(const QString &text)
{
    ui->scroll->setText(text);
}

void DlgCdg::stopTicker()
{
    ui->scroll->stop();
}

void DlgCdg::tickerFontChanged()
{
    ui->scroll->setFont(m_settings.tickerFont());
    ui->scroll->setMinimumHeight(QFontMetrics(m_settings.tickerFont()).height());
    ui->scroll->setMaximumHeight(QFontMetrics(m_settings.tickerFont()).height());
    ui->scroll->refresh();
}

void DlgCdg::tickerSpeedChanged()
{
    ui->scroll->setSpeed(m_settings.tickerSpeed());
}

void DlgCdg::tickerTextColorChanged()
{
    auto palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), m_settings.tickerTextColor());
    ui->scroll->setPalette(palette);
    ui->scroll->refresh();
}

void DlgCdg::tickerBgColorChanged()
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, m_settings.tickerBgColor());
    this->setPalette(palette);
    ui->scroll->refresh();
    //ui->scroll->refreshTickerSettings();
}

void DlgCdg::tickerEnableChanged()
{
    ui->scroll->setVisible(m_settings.tickerEnabled());
    ui->scroll->setTickerEnabled(m_settings.tickerEnabled());
    ui->scroll->setText(ui->scroll->getCurrentText(), true);
}

void DlgCdg::mouseDoubleClickEvent([[maybe_unused]]QMouseEvent *e)
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
        showFullScreen();
    else
        showNormal();
    cdgOffsetsChanged();
    m_settings.setCdgWindowFullscreen(m_fullScreen);
    m_settings.saveWindowState(this);
    QDesktopWidget widget;
    m_settings.setCdgWindowFullscreenMonitor(widget.screenNumber(this));
}

QFileInfoList DlgCdg::getSlideShowImages()
{
    QFileInfoList images;
    QDir srcDir(m_settings.bgSlideShowDir());
    auto files = srcDir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const auto & file : files)
    {
        if (QImageReader::imageFormat(file.absoluteFilePath()) != "")
            images << file;
    }
    return images;
}

void DlgCdg::showAlert(bool show)
{
    if ((show) && (m_settings.karaokeAAAlertEnabled()))
    {
        ui->videoDisplayKar->hide();
        ui->videoDisplayBm->hide();
        ui->widgetAlert->show();
    }
    else
    {
        ui->widgetAlert->hide();
        if (m_bmb.hasActiveVideo() && !m_kmb.hasActiveVideo()) {
            ui->videoDisplayBm->show();
            ui->videoDisplayKar->hide();
        }
        else {
            ui->videoDisplayBm->hide();
            ui->videoDisplayKar->show();
        }
    }
}

void DlgCdg::setNextSinger(const QString &name)
{
    ui->lblNextSinger->setText(name);
}

void DlgCdg::setNextSong(const QString &song)
{
    ui->lblNextSong->setText(song);
}

void DlgCdg::setCountdownSecs(int seconds)
{
    m_countdownPos = seconds;
    ui->lblSeconds->setText(QString::number(seconds) + tr(" seconds"));
    m_timerAlertCountdown.stop();
    m_timerAlertCountdown.start();
}

void DlgCdg::timerCountdownTimeout()
{
    if (m_countdownPos > 0)
        m_countdownPos--;
    ui->lblSeconds->setText(QString::number(m_countdownPos) + tr(" seconds"));
    ui->lblSeconds->repaint();
    ui->widgetAlert->repaint();
}

void DlgCdg::alertBgColorChanged(const QColor &color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->backgroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::alertTxtColorChanged(const QColor &color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->foregroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::applyBackgroundImageMode()
{
    if (m_settings.bgMode() == Settings::BgMode::BG_MODE_IMAGE && QFile::exists(m_settings.cdgDisplayBackgroundImage()))
    {
        m_timerSlideShow.stop();
        ui->videoDisplayKar->setBackground(QPixmap(m_settings.cdgDisplayBackgroundImage()));
    }
    else if (m_settings.bgMode() == Settings::BgMode::BG_MODE_SLIDESHOW && QDir(m_settings.bgSlideShowDir()).exists())
    {
        m_timerSlideShow.start();
        slideShowMoveNext();
    }
    else
    {
        m_timerSlideShow.stop();
        ui->videoDisplayKar->useDefaultBackground();
    }
}

void DlgCdg::timerSlideShowTimeout()
{
    if (!ui->videoDisplayKar->hasActiveVideo() && !ui->videoDisplayBm->hasActiveVideo())
    {
        slideShowMoveNext();
    }
}

void DlgCdg::slideShowMoveNext()
{
    m_curSlideshowPos++;
    auto images = getSlideShowImages();
    if (images.empty())
    {
        ui->videoDisplayKar->useDefaultBackground();
        return;
    }
    if (m_curSlideshowPos >= images.size())
        m_curSlideshowPos = 0;
    if (images.at(m_curSlideshowPos).fileName().endsWith("svg", Qt::CaseInsensitive))
    {
        QPixmap bgImage(QSize(1920,1080));
        QPainter painter(&bgImage);
        QSvgRenderer renderer(images.at(m_curSlideshowPos).absoluteFilePath());
        renderer.render(&painter);
        ui->videoDisplayKar->setBackground(bgImage);
    }
    else
        ui->videoDisplayKar->setBackground(images.at(m_curSlideshowPos).absoluteFilePath());
}

void DlgCdg::alertFontChanged(const QFont &font)
{
    ui->label->setFont(font);
    ui->label_2->setFont(font);
    ui->label_4->setFont(font);
    ui->lblNextSinger->setFont(font);
    ui->lblNextSong->setFont(font);
    ui->lblSeconds->setFont(font);
}

void DlgCdg::mouseMove([[maybe_unused]]QMouseEvent *event)
{
    if (m_fullScreen)
        ui->btnToggleFullscreen->setText(tr("Make Windowed"));
    else
        ui->btnToggleFullscreen->setText(tr("Make Fullscreen"));
    ui->fsToggleWidget->show();
    m_timerButtonShow.start();
}

void DlgCdg::timer1sTimeout()
{
    if (m_settings.cdgRemainEnabled())
    {
        if (m_kmb.state() == MediaBackend::PlayingState && !m_tWidget->isVisible())
        {
            m_tWidget->show();
        }
        else if (m_kmb.state() != MediaBackend::PlayingState && m_tWidget->isVisible())
        {
            m_tWidget->hide();
        }
        if (m_kmb.state() == MediaBackend::PlayingState)
        {
            m_tWidget->setString(" " + MediaBackend::msToMMSS(m_kmb.duration() - m_kmb.position()) + " ");
        }
    }
}


void DlgCdg::btnToggleFullscreenClicked()
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
    {
        showFullScreen();
    }
    else
        showNormal();
    m_settings.setCdgWindowFullscreen(m_fullScreen);
    m_settings.saveWindowState(this);
    QDesktopWidget widget;
    m_settings.setCdgWindowFullscreenMonitor(widget.screenNumber(this));
    cdgOffsetsChanged();
}

void DlgCdg::cdgOffsetsChanged()
{
    if (isFullScreen())
        this->layout()->setContentsMargins(m_settings.cdgOffsetLeft(),m_settings.cdgOffsetTop(),m_settings.cdgOffsetRight(),m_settings.cdgOffsetBottom());
    else
        this->layout()->setContentsMargins(0,0,0,0);
}


void DlgCdg::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    this->hide();
    m_settings.setShowCdgWindow(false);
    event->ignore();
    emit visibilityChanged(false);
}

void DlgCdg::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_settings.restoreWindowState(this);
    m_settings.setShowCdgWindow(true);
    if (m_settings.cdgWindowFullscreen())
    {
        this->showNormal();
        QTimer::singleShot(100, [&] () {
            ui->btnToggleFullscreen->setText("Make Windowed");
            this->showFullScreen();
            cdgOffsetsChanged();
        });

    }
    else
        ui->btnToggleFullscreen->setText("Make Fullscreen");
    QDialog::showEvent(event);
    emit visibilityChanged(true);
}

void DlgCdg::hideEvent(QHideEvent *event)
{
    m_settings.saveWindowState(this);
    QWidget::hideEvent(event);
}

void DlgCdg::setSlideshowInterval(int secs) {
    m_timerSlideShow.setInterval(secs * 1000);
}


TransparentWidget::~TransparentWidget()
{
    m_settings.saveWindowState(this);
}

void TransparentWidget::mouseMoveEvent(QMouseEvent *event)
{
    this->move(event->globalPos() + m_startPoint);
    m_settings.setDurationPosition(this->pos());
}

void TransparentWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    //settings.saveWindowState(this);
}

void TransparentWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPoint = frameGeometry().topLeft() - event->globalPos();
    }
}

void TransparentWidget::setString(const QString &string) const {
    m_label->setText(string);
}

void TransparentWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect (this->rect(), QColor(0, 0, 0, 0x20)); /* set transparent color*/
}

TransparentWidget::TransparentWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    auto layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    setContentsMargins(0,0,0,0);
    m_label = std::make_unique<QLabel>(this);
    layout->addWidget(m_label.get());
    m_label->setMargin(0);
    m_label->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_label->setText("00:00");
    m_label->setAutoFillBackground(true);
    m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void TransparentWidget::setTextColor(const QColor &color) const {
    auto palette = m_label->palette();
    palette.setColor(QPalette::WindowText, color);
    m_label->setPalette(palette);
}

void TransparentWidget::setBackgroundColor(const QColor &color) const {
    auto palette = m_label->palette();
    palette.setColor(QPalette::Window, color);
    m_label->setPalette(palette);
}

void TransparentWidget::setTextFont(const QFont &font) {
    m_label->setFont(font);
    m_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    setFixedSize(QFontMetrics(font).horizontalAdvance("_____"), QFontMetrics(font).height());
    m_label->setFixedSize(QFontMetrics(font).horizontalAdvance("_____"), QFontMetrics(font).height());
#else
    setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).tightBoundingRect("0123456789:").height());
    m_label->setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).tightBoundingRect("0123456789:").height());
#endif
}

void TransparentWidget::resetPosition() {
    move(0,0);
}
