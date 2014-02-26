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

#ifndef CDGWINDOW_H
#define CDGWINDOW_H

#include <QDialog>
#include <qglcanvas.h>
#include <QMouseEvent>
#include <khsettings.h>

namespace Ui {
class cdgWindow;
}

class CdgWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CdgWindow(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~CdgWindow();
    void updateCDG(QImage image);
    void makeFullscreen();
    void makeWindowed();
public slots:
    void setFullScreen(bool fullscreen);
    void setFullScreenMonitor(int monitor);

private:
    Ui::cdgWindow *ui;
    QGLCanvas *canvas;

private:
    bool m_fullScreen;
    QRect m_lastSize;
    KhSettings *settings;

    // QWidget interface
protected:
    void mouseDoubleClickEvent(QMouseEvent *e);
};

#endif // CDGWINDOW_H
