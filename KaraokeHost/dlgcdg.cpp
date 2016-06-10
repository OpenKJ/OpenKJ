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

extern KhSettings *settings;


DlgCdg::DlgCdg(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(new Ui::DlgCdg)
{
    hSizeAdjustment = settings->cdgHSizeAdjustment();
    vSizeAdjustment = settings->cdgVSizeAdjustment();
    hOffset = settings->cdgHOffset();
    vOffset = settings->cdgVOffset();
    ui->setupUi(this);
    cdgVideoSurface = new CdgVideoWidget(this);
    ui->verticalLayout_2->addWidget(cdgVideoSurface);
    m_fullScreen = false;
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
    ticker = new ScrollText(this);
    ticker->setFont(settings->tickerFont());
    ticker->setMinimumHeight(settings->tickerHeight());
    ticker->setMaximumHeight(settings->tickerHeight());
    ticker->setSpeed(settings->tickerSpeed());
    QPalette palette = ticker->palette();
    palette.setColor(ticker->foregroundRole(), settings->tickerTextColor());
    ticker->setPalette(palette);
    palette = this->palette();
    palette.setColor(QPalette::Background, settings->tickerBgColor());
    this->setPalette(palette);
    ticker->setText("This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll - This is some text to scroll");
    ui->verticalLayout_2->addWidget(ticker);
    connect(settings, SIGNAL(tickerFontChanged()), this, SLOT(tickerFontChanged()));
    connect(settings, SIGNAL(tickerHeightChanged(int)), this, SLOT(tickerHeightChanged()));
    connect(settings, SIGNAL(tickerSpeedChanged()), this, SLOT(tickerSpeedChanged()));
    connect(settings, SIGNAL(tickerTextColorChanged()), this, SLOT(tickerTextColorChanged()));
    connect(settings, SIGNAL(tickerBgColorChanged()), this, SLOT(tickerBgColorChanged()));
    connect(settings, SIGNAL(tickerEnableChanged()), this, SLOT(tickerEnableChanged()));
    connect(settings, SIGNAL(cdgBgImageChanged()), cdgVideoSurface, SLOT(presentBgImage()));
    connect(settings, SIGNAL(cdgHOffsetChanged(int)), this, SLOT(setHOffset(int)));
    connect(settings, SIGNAL(cdgVOffsetChanged(int)), this, SLOT(setVOffset(int)));
    connect(settings, SIGNAL(cdgHSizeAdjustmentChanged(int)), this, SLOT(setHSizeAdjustment(int)));
    connect(settings, SIGNAL(cdgVSizeAdjustmentChanged(int)), this, SLOT(setVSizeAdjustment(int)));
    connect(settings, SIGNAL(cdgShowCdgWindowChanged(bool)), this, SLOT(setVisible(bool)));
    connect(settings, SIGNAL(cdgWindowFullscreenChanged(bool)), this, SLOT(setFullScreen(bool)));
    fullScreenTimer = new QTimer(this);
    connect(fullScreenTimer, SIGNAL(timeout()), this, SLOT(fullScreenTimerTimeout()));
    fullScreenTimer->setInterval(500);
    cdgVideoSurface->videoSurface()->start();
    cdgVideoSurface->setUseBgImage(true);
    if ((settings->cdgWindowFullscreen()) && (settings->showCdgWindow()))
    {
        makeFullscreen();
    }
    else if (settings->showCdgWindow())
        show();
    fullScreenTimer = new QTimer(this);
    connect(fullScreenTimer, SIGNAL(timeout()), this, SLOT(fullScreenTimerTimeout()));
    fullScreenTimer->setInterval(500);
}

DlgCdg::~DlgCdg()
{
    delete ui;
}

void DlgCdg::updateCDG(QImage image, bool overrideVisibleCheck)
{
    if ((isVisible()) || (overrideVisibleCheck))
    {
        if (image.size().height() > cdgVideoSurface->size().height() || image.size().width() > cdgVideoSurface->size().width())
            cdgVideoSurface->videoSurface()->present(QVideoFrame(image.scaled(cdgVideoSurface->size(), Qt::IgnoreAspectRatio)));
        else
            cdgVideoSurface->videoSurface()->present(QVideoFrame(image));
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
//    flags |= Qt::WindowStaysOnTopHint;
  //  flags |= Qt::MaximizeUsingFullscreenGeometryHint;
    setWindowFlags(flags);
    QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
    move(screenDimensions.left()  + hOffset, screenDimensions.top() + vOffset);
    resize(screenDimensions.width() + hSizeAdjustment,screenDimensions.height() + vSizeAdjustment);
    cdgVideoSurface->presentBgImage();
    show();
    fullScreenTimer->start();
}

void DlgCdg::makeWindowed()
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    resize(300, 216);
    show();
    cdgVideoSurface->repaint();
    m_fullScreen = false;
}

void DlgCdg::setTickerText(QString text)
{
    ticker->setText(text);
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
    makeWindowed();
    makeFullscreen();
}

void DlgCdg::tickerFontChanged()
{
    ticker->setFont(settings->tickerFont());
    ticker->refresh();
    int newHeight = QFontMetrics(ticker->font()).height() * 1.2;
    settings->setTickerHeight(newHeight);
}

void DlgCdg::tickerHeightChanged()
{
    ticker->setMinimumHeight(settings->tickerHeight());
    ticker->setMaximumHeight(settings->tickerHeight());
    ticker->refresh();
}

void DlgCdg::tickerSpeedChanged()
{
    ticker->setSpeed(settings->tickerSpeed());
}

void DlgCdg::tickerTextColorChanged()
{
    QPalette palette = ticker->palette();
    palette.setColor(ticker->foregroundRole(), settings->tickerTextColor());
    ticker->setPalette(palette);
}

void DlgCdg::tickerBgColorChanged()
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, settings->tickerBgColor());
    this->setPalette(palette);
}

void DlgCdg::tickerEnableChanged()
{
    ticker->enable(settings->tickerEnabled());
}

void DlgCdg::presentBgImage()
{
    cdgVideoSurface->presentBgImage();
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
    setVOffset(settings->cdgVOffset());
    setHOffset(settings->cdgHOffset());
    fullScreenTimer->stop();
}

