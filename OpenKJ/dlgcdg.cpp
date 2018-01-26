/*
 * Copyright (c) 2013-2016 Thomas Isaac Lightburn
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
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QSvgRenderer>
#include <QPainter>
#include <QDir>
#include <QImageReader>

extern Settings *settings;


void DlgCdg::setKAudioBackend(AbstractAudioBackend *value)
{
    kAudioBackend = value;
}

void DlgCdg::setBAudioBackend(AbstractAudioBackend *value)
{
    bAudioBackend = value;
}

DlgCdg::DlgCdg(AbstractAudioBackend *KaraokeBackend, AbstractAudioBackend *BreakBackend, QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(new Ui::DlgCdg)
{
    kAudioBackend = KaraokeBackend;
    bAudioBackend = BreakBackend;
    if (settings->cdgWindowFullScreenMonitor() > QApplication::desktop()->numScreens())
    {
        settings->setCdgWindowFullscreen(false);
    }
    hSizeAdjustment = settings->cdgHSizeAdjustment();
    vSizeAdjustment = settings->cdgVSizeAdjustment();
    hOffset = settings->cdgHOffset();
    vOffset = settings->cdgVOffset();
    ui->setupUi(this);
    m_fullScreen = false;
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
    ui->scroll->setFont(settings->tickerFont());
    ui->scroll->setMinimumHeight(settings->tickerHeight());
    ui->scroll->setMaximumHeight(settings->tickerHeight());
    ui->scroll->setSpeed(settings->tickerSpeed());
    QPalette palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), settings->tickerTextColor());
    ui->scroll->setPalette(palette);
    palette = this->palette();
    palette.setColor(QPalette::Background, settings->tickerBgColor());
    this->setPalette(palette);
    ui->scroll->setText("This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll");
    connect(settings, SIGNAL(tickerFontChanged()), this, SLOT(tickerFontChanged()));
    connect(settings, SIGNAL(tickerHeightChanged(int)), this, SLOT(tickerHeightChanged()));
    connect(settings, SIGNAL(tickerSpeedChanged()), this, SLOT(tickerSpeedChanged()));
    connect(settings, SIGNAL(tickerTextColorChanged()), this, SLOT(tickerTextColorChanged()));
    connect(settings, SIGNAL(tickerBgColorChanged()), this, SLOT(tickerBgColorChanged()));
    connect(settings, SIGNAL(tickerEnableChanged()), this, SLOT(tickerEnableChanged()));
    connect(settings, SIGNAL(cdgHOffsetChanged(int)), this, SLOT(setHOffset(int)));
    connect(settings, SIGNAL(cdgVOffsetChanged(int)), this, SLOT(setVOffset(int)));
    connect(settings, SIGNAL(cdgHSizeAdjustmentChanged(int)), this, SLOT(setHSizeAdjustment(int)));
    connect(settings, SIGNAL(cdgVSizeAdjustmentChanged(int)), this, SLOT(setVSizeAdjustment(int)));
    connect(settings, SIGNAL(cdgShowCdgWindowChanged(bool)), this, SLOT(setVisible(bool)));
    connect(settings, SIGNAL(cdgWindowFullscreenChanged(bool)), this, SLOT(setFullScreen(bool)));
    connect(settings, SIGNAL(cdgWindowFullscreenMonitorChanged(int)), this, SLOT(setFullScreenMonitor(int)));
    connect(ui->cdgVideo, SIGNAL(resized(QSize)), this, SLOT(cdgSurfaceResized(QSize)));
    connect(settings, SIGNAL(karaokeAAAlertFontChanged(QFont)), this, SLOT(alertFontChanged(QFont)));
    fullScreenTimer = new QTimer(this);
    slideShowTimer = new QTimer(this);
    connect(fullScreenTimer, SIGNAL(timeout()), this, SLOT(fullScreenTimerTimeout()));
    connect(slideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowTimerTimeout()));
    slideShowTimer->start(15000);
    fullScreenTimer->setInterval(500);
    ui->cdgVideo->videoSurface()->start();
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        makeFullscreen();
    }
    else if (settings->showCdgWindow())
        show();
    else
        hide();
    setShowBgImage(true);
    slideShowTimerTimeout();
    showAlert(false);
    countdownPos = 0;
    alertCountdownTimer = new QTimer(this);
    alertCountdownTimer->setInterval(1000);
    connect(alertCountdownTimer, SIGNAL(timeout()), this, SLOT(countdownTimerTimeout()));
    ui->widgetAlert->setAutoFillBackground(true);
    alertFontChanged(settings->karaokeAAAlertFont());
    ui->cdgVideo->setMouseTracking(true);
    ui->fsToggleWidget->hide();
    ui->widgetAlert->hide();
    ui->widgetAlert->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->widgetAlert->setMouseTracking(true);
    connect(ui->cdgVideo, SIGNAL(mouseMoveEvent(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
    buttonShowTimer = new QTimer(this);
    buttonShowTimer->setInterval(1000);
    connect(buttonShowTimer, SIGNAL(timeout()), this, SLOT(buttonShowTimerTimeout()));
    alertBgColorChanged(settings->alertBgColor());
    alertTxtColorChanged(settings->alertTxtColor());
    connect(settings, SIGNAL(alertBgColorChanged(QColor)), this, SLOT(alertBgColorChanged(QColor)));
    connect(settings, SIGNAL(alertTxtColorChanged(QColor)), this, SLOT(alertTxtColorChanged(QColor)));
    connect(kAudioBackend, SIGNAL(stateChanged(AbstractAudioBackend::State)), this, SLOT(triggerBg(AbstractAudioBackend::State)));
    connect(settings, SIGNAL(bgModeChanged(BgMode)), this, SLOT(triggerBg()));

}

DlgCdg::~DlgCdg()
{
    delete ui;
}

void DlgCdg::updateCDG(QImage image, bool overrideVisibleCheck)
{
    if ((isVisible()) || (overrideVisibleCheck))
    {
     //   if (image.size().height() > ui->cdgVideo->size().height() || image.size().width() > ui->cdgVideo->size().width())
     //       ui->cdgVideo->videoSurface()->present(QVideoFrame(image));
     //       ui->cdgVideo->videoSurface()->present(QVideoFrame(image.scaled(ui->cdgVideo->size(), Qt::IgnoreAspectRatio)));
     //   else
            ui->cdgVideo->videoSurface()->present(QVideoFrame(image));
    }
}

void DlgCdg::makeFullscreen()
{
    m_fullScreen = true;
    m_lastSize.setHeight(height());
    m_lastSize.setWidth(width());
    Qt::WindowFlags flags;
    flags |= Qt::Window;
    flags |= Qt::FramelessWindowHint;
    setWindowFlags(flags);
    QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
    move(screenDimensions.left()  + hOffset, screenDimensions.top() + vOffset);
    resize(screenDimensions.width() + hSizeAdjustment,screenDimensions.height() + vSizeAdjustment);
    show();
    fullScreenTimer->start();
}

void DlgCdg::makeWindowed()
{
    setWindowFlags(Qt::Window);
    resize(300, 216);
    if (settings->showCdgWindow())
        show();
    ui->cdgVideo->repaint();
    m_fullScreen = false;
}

void DlgCdg::setTickerText(QString text)
{
    ui->scroll->setText(text);
}

void DlgCdg::setFullScreen(bool fullscreen)
{
    if (fullscreen)
        makeFullscreen();
    else
        makeWindowed();
}

void DlgCdg::setFullScreenMonitor(int monitor)
{
    Q_UNUSED(monitor);
    if (settings->cdgWindowFullscreen())
    {
        makeWindowed();
        makeFullscreen();
    }
}

void DlgCdg::tickerFontChanged()
{
    ui->scroll->setFont(settings->tickerFont());
    ui->scroll->refresh();
    int newHeight = QFontMetrics(ui->scroll->font()).height() * 1.2;
    settings->setTickerHeight(newHeight);
}

void DlgCdg::tickerHeightChanged()
{
    ui->scroll->setMinimumHeight(settings->tickerHeight());
    ui->scroll->setMaximumHeight(settings->tickerHeight());
    ui->scroll->refresh();
}

void DlgCdg::tickerSpeedChanged()
{
    ui->scroll->setSpeed(settings->tickerSpeed());
}

void DlgCdg::tickerTextColorChanged()
{
    QPalette palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), settings->tickerTextColor());
    ui->scroll->setPalette(palette);
}

void DlgCdg::tickerBgColorChanged()
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, settings->tickerBgColor());
    this->setPalette(palette);
}

void DlgCdg::tickerEnableChanged()
{
    ui->scroll->enable(settings->tickerEnabled());
}

void DlgCdg::setVOffset(int pixels)
{
    vOffset = pixels;
    if (m_fullScreen)
    {
        QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
        move(screenDimensions.left()  + hOffset, screenDimensions.top() + vOffset);
    }
}

void DlgCdg::setHOffset(int pixels)
{
    hOffset = pixels;
    if (m_fullScreen)
    {
        QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
        move(screenDimensions.left()  + hOffset, screenDimensions.top() + vOffset);
    }
}

void DlgCdg::setVSizeAdjustment(int pixels)
{
    vSizeAdjustment = pixels;
    if (m_fullScreen)
    {
        QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
        resize(screenDimensions.width() + hSizeAdjustment,screenDimensions.height() + vSizeAdjustment);
    }
}

void DlgCdg::setHSizeAdjustment(int pixels)
{
    hSizeAdjustment = pixels;
    if (m_fullScreen)
    {
        QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
        resize(screenDimensions.width() + hSizeAdjustment,screenDimensions.height() + vSizeAdjustment);
    }
}

void DlgCdg::setShowBgImage(bool show)
{
    qWarning() << "DlgCdg::setShowBgImage(" << show << ") called";
    showBgImage = show;
    if ((show) && (settings->bgMode() == settings->BG_MODE_IMAGE))
    {
        if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
            return;
        if (bAudioBackend->state() == AbstractAudioBackend::PlayingState && bAudioBackend->hasVideo())
            return;
        if (settings->cdgDisplayBackgroundImage() != QString::null)
            ui->cdgVideo->videoSurface()->present(QVideoFrame(QImage(settings->cdgDisplayBackgroundImage())));
        else
        {
            QImage bgImage(ui->cdgVideo->size(), QImage::Format_ARGB32);
            QPainter painter(&bgImage);
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
            renderer.render(&painter);
            ui->cdgVideo->videoSurface()->present(QVideoFrame(bgImage));
        }

    }
}

void DlgCdg::cdgSurfaceResized(QSize size)
{
    Q_UNUSED(size)
    setShowBgImage(true);
}



void DlgCdg::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if (m_fullScreen)
    {
        makeWindowed();
    }
    else
        makeFullscreen();
}

void DlgCdg::fullScreenTimerTimeout()
{
    // This is to work around Windows 10 opening the window offset from the top left corner unless we wait a bit
    // before moving it.
    if ((settings->showCdgWindow()) && (settings->cdgWindowFullscreen()))
    {
        setVOffset(settings->cdgVOffset());
        setHOffset(settings->cdgHOffset());
        ui->cdgVideo->repaint();
        fullScreenTimer->stop();
    }
}

QFileInfoList DlgCdg::getSlideShowImages()
{
    QFileInfoList images;
    QDir srcDir(settings->bgSlideShowDir());
    QFileInfoList files = srcDir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (int i=0; i < files.size(); i++)
    {
        if (QImageReader::imageFormat(files.at(i).absoluteFilePath()) != "")
            images << files.at(i);
    }
    return images;
}

void DlgCdg::setAlert(QString text)
{
    Q_UNUSED(text)
    //ui->lblAlert->setText(text);
}

void DlgCdg::showAlert(bool show)
{
    if ((show) && (settings->karaokeAAAlertEnabled()))
    {
        ui->cdgVideo->hide();
        ui->widgetAlert->show();
    }
    else
    {
        ui->widgetAlert->hide();
        ui->cdgVideo->show();
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
    countdownPos = seconds;
    ui->lblSeconds->setText(QString::number(seconds) + " seconds");
    alertCountdownTimer->stop();
    alertCountdownTimer->start();
}

void DlgCdg::countdownTimerTimeout()
{
    if (countdownPos > 0)
        countdownPos--;
    ui->lblSeconds->setText(QString::number(countdownPos) + " seconds");
    ui->lblSeconds->repaint();
    ui->widgetAlert->repaint();
}

void DlgCdg::alertBgColorChanged(QColor color)
{
    QPalette palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->backgroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::alertTxtColorChanged(QColor color)
{
    QPalette palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->foregroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::triggerBg()
{
        showBgImage = true;
        qWarning() << "triggerBg called";
        slideShowTimerTimeout();
        setShowBgImage(true);
}

void DlgCdg::slideShowTimerTimeout()
{
    if ((showBgImage) && (settings->bgMode() == settings->BG_MODE_SLIDESHOW))
    {
        if (kAudioBackend->state() == AbstractAudioBackend::PlayingState)
            return;
        if (bAudioBackend->state() == AbstractAudioBackend::PlayingState && bAudioBackend->hasVideo())
            return;
        static int position = 0;
        QFileInfoList images = getSlideShowImages();
        if (images.size() == 0)
        {
            QImage bgImage(ui->cdgVideo->size(), QImage::Format_ARGB32);
            QPainter painter(&bgImage);
            QSvgRenderer renderer(QString(":icons/Icons/okjlogo.svg"));
            renderer.render(&painter);
            ui->cdgVideo->videoSurface()->present(QVideoFrame(bgImage));
            return;
        }
        if (position >= images.size())
            position = 0;
        if (images.at(position).fileName().endsWith("svg", Qt::CaseInsensitive))
        {
            QImage bgImage(ui->cdgVideo->size(), QImage::Format_ARGB32);
            QPainter painter(&bgImage);
            QSvgRenderer renderer(images.at(position).absoluteFilePath());
            renderer.render(&painter);
            ui->cdgVideo->videoSurface()->present(QVideoFrame(bgImage));
        }
        else
            ui->cdgVideo->videoSurface()->present(QVideoFrame(QImage(images.at(position).absoluteFilePath())));
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

void DlgCdg::mouseMove(QMouseEvent *event)
{
    qWarning() << "Mouse moved pos:" << event->pos();
    if (m_fullScreen)
        ui->btnToggleFullscreen->setText("Make Windowed");
    else
        ui->btnToggleFullscreen->setText("Make Fullscreen");
    ui->fsToggleWidget->show();
    buttonShowTimer->start();
}

void DlgCdg::buttonShowTimerTimeout()
{
    ui->fsToggleWidget->hide();
}


void DlgCdg::on_btnToggleFullscreen_clicked()
{
    if (m_fullScreen)
    {
        makeWindowed();
    }
    else
        makeFullscreen();
}
