/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#include "cdgwindow.h"
#include "ui_cdgwindow.h"
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>

CdgWindow::CdgWindow(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(new Ui::cdgWindow)
{
    ui->setupUi(this);
    settings = new KhSettings(this);
    canvas = new QGLCanvas(this);
    ui->verticalLayout->addWidget(canvas);
    canvas->repaint();
    m_fullScreen = false;
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
}

CdgWindow::~CdgWindow()
{
    delete ui;
}

void CdgWindow::updateCDG(QImage image)
{
    canvas->setImage(image);
    canvas->repaint();
}

void CdgWindow::makeFullscreen()
{
    m_lastSize.setHeight(height());
    m_lastSize.setWidth(width());
    Qt::WindowFlags flags;
    flags |= Qt::Window;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    move(QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor()).topLeft());
    QRect screenDimensions = QApplication::desktop()->screenGeometry(settings->cdgWindowFullScreenMonitor());
    resize(screenDimensions.width(),screenDimensions.height());
    show();
    updateCDG(QImage(":/icons/Icons/openkjlogo1.png"));
    m_fullScreen = true;
}

void CdgWindow::makeWindowed()
{
//    hide();
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    resize(300, 216);
    settings->saveWindowState(this);
    show();
    canvas->repaint();
    m_fullScreen = false;
}

void CdgWindow::setFullScreen(bool fullscreen)
{
    if (fullscreen)
        makeFullscreen();
    else
        makeWindowed();
}

void CdgWindow::setFullScreenMonitor(int monitor)
{
    Q_UNUSED(monitor);
    makeWindowed();
    makeFullscreen();
}

void CdgWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if (m_fullScreen)
    {
        makeWindowed();
    }
    else
        makeFullscreen();
}

