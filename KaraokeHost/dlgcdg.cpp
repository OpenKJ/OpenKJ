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

extern Settings *settings;


DlgCdg::DlgCdg(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(new Ui::DlgCdg)
{
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
    fullScreenTimer = new QTimer(this);
    setShowBgImage(true);
    connect(fullScreenTimer, SIGNAL(timeout()), this, SLOT(fullScreenTimerTimeout()));
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

}

DlgCdg::~DlgCdg()
{
    delete ui;
}

void DlgCdg::updateCDG(QImage image, bool overrideVisibleCheck)
{
    if ((isVisible()) || (overrideVisibleCheck))
    {
        if (image.size().height() > ui->cdgVideo->size().height() || image.size().width() > ui->cdgVideo->size().width())
            ui->cdgVideo->videoSurface()->present(QVideoFrame(image.scaled(ui->cdgVideo->size(), Qt::IgnoreAspectRatio)));
        else
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
    makeWindowed();
    makeFullscreen();
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
    if (show)
    {
        if (settings->cdgDisplayBackgroundImage() != QString::null)
            ui->cdgVideo->videoSurface()->present(QVideoFrame(QImage(settings->cdgDisplayBackgroundImage())));
        else
            ui->cdgVideo->videoSurface()->present(QVideoFrame(QImage(":/icons/Icons/openkjlogo1.png")));
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
    if ((settings->showCdgWindow()) && (settings->cdgWindowFullscreen()))
    {
        setVOffset(settings->cdgVOffset());
        setHOffset(settings->cdgHOffset());
        fullScreenTimer->stop();
    }
}

