/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
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
#include <QGuiApplication>
#include <QDesktopWidget>
#include <QSvgRenderer>
#include <QPainter>
#include <QDir>
#include <QImageReader>
#include <QScreen>

extern Settings *settings;



WId DlgCdg::getCdgWinId()
{
    return ui->videoDisplay->winId();
}

DlgCdg::DlgCdg(MediaBackend *KaraokeBackend, MediaBackend *BreakBackend, QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f), ui(new Ui::DlgCdg), m_kmb(KaraokeBackend), m_bmb(BreakBackend)
{
    ui->setupUi(this);
    tWidget = new TransparentWidget(this);
    tWidget->show();
    settings->restoreWindowState(tWidget);
    ui->videoDisplay->setMediaBackends(m_kmb, m_bmb);
    ui->widgetAlert->setAutoFillBackground(true);
    ui->fsToggleWidget->hide();
    ui->widgetAlert->hide();
    ui->widgetAlert->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->widgetAlert->setMouseTracking(true);
    ui->scroll->setVisible(settings->tickerEnabled());
    ui->scroll->setTickerEnabled(settings->tickerEnabled());
    tWidget->setVisible(settings->cdgRemainEnabled());
    ui->scroll->setFont(settings->tickerFont());
    ui->scroll->setMinimumHeight(settings->tickerHeight());
    ui->scroll->setMaximumHeight(settings->tickerHeight());
    ui->scroll->setSpeed(settings->tickerSpeed());
    cdgRemainFontChanged(settings->cdgRemainFont());
    QPalette palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), settings->tickerTextColor());
    ui->scroll->setPalette(palette);
    palette = this->palette();
    palette.setColor(QPalette::Window, settings->tickerBgColor());
    setPalette(palette);
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
    m_fullScreen = settings->cdgWindowFullscreen();
    cdgRemainTextColorChanged(settings->cdgRemainTextColor());
    cdgRemainBgColorChanged(settings->cdgRemainBgColor());
    setShowBgImage(true);
    timerSlideShowTimeout();
    showAlert(false);
    alertFontChanged(settings->karaokeAAAlertFont());
    alertBgColorChanged(settings->alertBgColor());
    alertTxtColorChanged(settings->alertTxtColor());
    cdgOffsetsChanged();

    connect(ui->videoDisplay, &VideoDisplay::mouseMoveEvent, this, &DlgCdg::mouseMove);
    connect(settings, &Settings::alertBgColorChanged, this, &DlgCdg::alertBgColorChanged);
    connect(settings, &Settings::alertTxtColorChanged, this, &DlgCdg::alertTxtColorChanged);
    connect(settings, &Settings::bgModeChanged, [&] () {triggerBg();});
    connect(settings, &Settings::cdgRemainEnabledChanged, tWidget, &QWidget::setVisible);
    connect(settings, &Settings::cdgOffsetsChanged, this, &DlgCdg::cdgOffsetsChanged);
    connect(settings, &Settings::tickerFontChanged, this, &DlgCdg::tickerFontChanged);
    connect(settings, &Settings::tickerHeightChanged, this, &DlgCdg::tickerHeightChanged);
    connect(settings, &Settings::tickerSpeedChanged, this, &DlgCdg::tickerSpeedChanged);
    connect(settings, &Settings::tickerTextColorChanged, this, &DlgCdg::tickerTextColorChanged);
    connect(settings, &Settings::tickerBgColorChanged, this, &DlgCdg::tickerBgColorChanged);
    connect(settings, &Settings::tickerEnableChanged, this, &DlgCdg::tickerEnableChanged);
    connect(settings, &Settings::cdgRemainFontChanged, this, &DlgCdg::cdgRemainFontChanged);
    connect(settings, &Settings::cdgRemainTextColorChanged, this, &DlgCdg::cdgRemainTextColorChanged);
    connect(settings, &Settings::cdgRemainBgColorChanged, this, &DlgCdg::cdgRemainBgColorChanged);
    connect(settings, &Settings::karaokeAAAlertFontChanged, this, &DlgCdg::alertFontChanged);
    connect(settings, &Settings::durationPositionReset, [&] () {
       tWidget->move(0,0);
    });
    connect(&m_timerSlideShow, &QTimer::timeout, this, &DlgCdg::timerSlideShowTimeout);
    connect(&m_timer1s, &QTimer::timeout, this, &DlgCdg::timer1sTimeout);
    connect(&m_timerAlertCountdown, &QTimer::timeout, this, &DlgCdg::timerCountdownTimeout);
    connect(&m_timerButtonShow, &QTimer::timeout, [&] () { ui->fsToggleWidget->hide(); });
    m_timerButtonShow.setInterval(1000);
    m_timerAlertCountdown.setInterval(1000);
    m_timer1s.start(1000);
    m_timerSlideShow.start(15000);
    if (!settings->showCdgWindow())
        hide();
    else
        show();
}

DlgCdg::~DlgCdg()
{
    tWidget->deleteLater();
    delete ui;
}

void DlgCdg::setTickerText(QString text)
{
    ui->scroll->setText(text);
}

void DlgCdg::stopTicker()
{
    ui->scroll->stop();
}

void DlgCdg::tickerFontChanged()
{
    //ui->scroll->refreshTickerSettings();
    ui->scroll->setFont(settings->tickerFont());
    settings->setTickerHeight(QFontMetrics(ui->scroll->font()).height());
}

void DlgCdg::tickerHeightChanged(const int &height)
{
    ui->scroll->setMinimumHeight(height);
    ui->scroll->setMaximumHeight(height);
}

void DlgCdg::tickerSpeedChanged()
{
    ui->scroll->setSpeed(settings->tickerSpeed());
}

void DlgCdg::tickerTextColorChanged()
{
    auto palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), settings->tickerTextColor());
    ui->scroll->setPalette(palette);
    ui->scroll->refresh();
    //ui->scroll->refreshTickerSettings();
}

void DlgCdg::tickerBgColorChanged()
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, settings->tickerBgColor());
    this->setPalette(palette);
    ui->scroll->refresh();
    //ui->scroll->refreshTickerSettings();
}

void DlgCdg::tickerEnableChanged()
{
    ui->scroll->setVisible(settings->tickerEnabled());
    ui->scroll->setTickerEnabled(settings->tickerEnabled());
}

void DlgCdg::cdgRemainFontChanged(QFont font)
{
    tWidget->label->setFont(font);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    tWidget->setFixedSize(QFontMetrics(font).horizontalAdvance("_____"),QFontMetrics(font).height());
    tWidget->label->setFixedSize(QFontMetrics(font).horizontalAdvance("_____"),QFontMetrics(font).height());
#else
    tWidget->setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).height());
    tWidget->label->setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).height());
#endif
}

void DlgCdg::cdgRemainTextColorChanged(QColor color)
{
    auto palette = tWidget->label->palette();
    palette.setColor(QPalette::WindowText, color);
    tWidget->label->setPalette(palette);
}

void DlgCdg::cdgRemainBgColorChanged(QColor color)
{
    auto palette = tWidget->label->palette();
    palette.setColor(QPalette::Window, color);
    tWidget->label->setPalette(palette);
}

void DlgCdg::setShowBgImage(bool show)
{
    m_showBgImage = show;
    if ((show) && (settings->bgMode() == Settings::BgMode::BG_MODE_IMAGE))
    {
        if (settings->cdgDisplayBackgroundImage() != QString())
            ui->videoDisplay->setBackground(QPixmap(settings->cdgDisplayBackgroundImage()));
        else
        {
            QPixmap bgImage(QSize(1920,1080));
            QPainter painter(&bgImage);
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
            renderer.render(&painter);
            ui->videoDisplay->setBackground(bgImage);
        }
    }
}

void DlgCdg::mouseDoubleClickEvent([[maybe_unused]]QMouseEvent *e)
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
        showFullScreen();
    else
        showNormal();
    cdgOffsetsChanged();
    settings->setCdgWindowFullscreen(m_fullScreen);
    settings->saveWindowState(this);
    QDesktopWidget widget;
    settings->setCdgWindowFullscreenMonitor(widget.screenNumber(this));
}

QFileInfoList DlgCdg::getSlideShowImages()
{
    QFileInfoList images;
    QDir srcDir(settings->bgSlideShowDir());
    auto files = srcDir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (int i=0; i < files.size(); i++)
    {
        if (QImageReader::imageFormat(files.at(i).absoluteFilePath()) != "")
            images << files.at(i);
    }
    return images;
}

void DlgCdg::showAlert(bool show)
{
    if ((show) && (settings->karaokeAAAlertEnabled()))
    {
        ui->videoDisplay->hide();
        ui->widgetAlert->show();
    }
    else
    {
        ui->widgetAlert->hide();
        ui->videoDisplay->show();
    }
}

void DlgCdg::setNextSinger(QString name)
{
    ui->lblNextSinger->setText(name);
}

void DlgCdg::setNextSong(QString song)
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

void DlgCdg::alertBgColorChanged(QColor color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->backgroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::alertTxtColorChanged(QColor color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->foregroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::triggerBg()
{
    timerSlideShowTimeout();
    setShowBgImage(true);
}

void DlgCdg::cdgRemainEnabledChanged(bool enabled)
{
    if ((m_kmb->state() == MediaBackend::PlayingState) || !enabled)
    {
        tWidget->setVisible(enabled);
    }
}

void DlgCdg::timerSlideShowTimeout()
{
    if ((m_showBgImage) && (settings->bgMode() == Settings::BgMode::BG_MODE_SLIDESHOW))
    {
        static int position = 0;
        auto images = getSlideShowImages();
        if (images.size() == 0)
        {
            QPixmap bgImage(QSize(1920,1080));
            QPainter painter(&bgImage);
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
            renderer.render(&painter);
            ui->videoDisplay->setBackground(QPixmap(bgImage));
            return;
        }
        if (position >= images.size())
            position = 0;
        if (images.at(position).fileName().endsWith("svg", Qt::CaseInsensitive))
        {
            QPixmap bgImage(QSize(1920,1080));
            QPainter painter(&bgImage);
            QSvgRenderer renderer(images.at(position).absoluteFilePath());
            renderer.render(&painter);
            ui->videoDisplay->setBackground(bgImage);
        }
        else
            ui->videoDisplay->setBackground(images.at(position).absoluteFilePath());
        position++;

    }
}

void DlgCdg::alertFontChanged(QFont font)
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
    if (settings->cdgRemainEnabled())
    {
        if (m_kmb->state() == MediaBackend::PlayingState && !tWidget->isVisible())
        {
            tWidget->show();
        }
        else if (m_kmb->state() != MediaBackend::PlayingState && tWidget->isVisible())
        {
            tWidget->hide();
        }
        if (m_kmb->state() == MediaBackend::PlayingState)
        {
            tWidget->setString(" " + m_kmb->msToMMSS(m_kmb->duration() - m_kmb->position()) + " ");
        }
    }
}


void DlgCdg::on_btnToggleFullscreen_clicked()
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
    {
        showFullScreen();
    }
    else
        showNormal();
    settings->setCdgWindowFullscreen(m_fullScreen);
    settings->saveWindowState(this);
    QDesktopWidget widget;
    settings->setCdgWindowFullscreenMonitor(widget.screenNumber(this));
    cdgOffsetsChanged();
}

void DlgCdg::cdgOffsetsChanged()
{
    if (isFullScreen())
        this->layout()->setContentsMargins(settings->cdgOffsetLeft(),settings->cdgOffsetTop(),settings->cdgOffsetRight(),settings->cdgOffsetBottom());
    else
        this->layout()->setContentsMargins(0,0,0,0);
}


void DlgCdg::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    this->hide();
    settings->setShowCdgWindow(false);
    event->ignore();
}

void DlgCdg::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    settings->restoreWindowState(this);
    settings->setShowCdgWindow(true);
    if (settings->cdgWindowFullscreen())
    {
        ui->btnToggleFullscreen->setText("Make Windowed");
        this->showFullScreen();
        cdgOffsetsChanged();
    }
    else
        ui->btnToggleFullscreen->setText("Make Fullscreen");
}

void DlgCdg::hideEvent(QHideEvent *event)
{
    settings->saveWindowState(this);
    QWidget::hideEvent(event);
}


void TransparentWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    settings->saveWindowState(this);
}
